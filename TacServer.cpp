#include "TacServer.h"

#include "HexTools.h"

#include <iostream>

namespace Tac {

int IdGenerator::generate(void) {
    const int maxId = 100000; // should not approach 2**32 -- then just drop the modulus
    int rv = 0;
    while( rv <= 0 || find( usedIds.begin(), usedIds.end(), rv ) != usedIds.end() ) {
        rv = 1 + abs( prng() ) % maxId;
    }
    addUsed( rv );
    return rv;
}

void IdGenerator::addUsed(int id) {
    usedIds.insert( id );
}

ServerPlayer::ServerPlayer(SProto::Server& server, ServerMap& smap, int id, const std::string& username) :
    id ( id ),
    username ( username ),
    memory ( smap.getMapSize() ),
    server ( server ),
    smap ( smap )
{
    using namespace std;
    int sz = memory.getSize();
    memory.getDefault() = 0;
    for(int i=0;i<sz;i++) {
        memory.get(i) = 0;
    }
}

ServerTile::ServerTile(void) :
    tileType(  0 )
{
    for(int i=0;i<UNIT_LAYERS;i++) {
        units[i] = 0;
    }
    // map will initialize xy when appropriate -- no sense destroying the valgrind
    // warnings if we miss that
}

ServerMap::ServerMap(int mapSize, TileType *defaultTt) :
    mapSize ( mapSize ),
    unitIdGen (),
    playerIdGen (),
    tiles ( mapSize ),
    players (),
    units ()
{
    using namespace std;
    for(int r=1;r<=mapSize;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++) {
        int x, y;
        HexTools::cartesianiseHexCoordinate( i, j, r, x, y );
        tiles.get(x,y).setXY(x,y);
        tiles.get(x,y).setTileType( defaultTt );
    }
    tiles.get(0,0).setXY(0,0);
    tiles.getDefault().setTileType( defaultTt );
}

ServerUnit::ServerUnit(int id, const UnitType& unitType) :
    id ( id ),
    unitType ( unitType ),
    controller ( 0 ),
    tile ( 0 )
{
}

void trivialLevelGenerator(ServerMap& smap, TileType* wall, TileType* floor, double wallDensity) {
    const int mapSize = smap.getMapSize();
    MTRand prng;
    smap.getTile(0,0).setTileType( (prng() < wallDensity) ? wall : floor );
    for(int r=1;r<=mapSize;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++) {
        int x, y;
        HexTools::cartesianiseHexCoordinate( i, j, r, x, y );
        smap.getTile(x,y).setTileType( (prng() < wallDensity) ? wall : floor );
    }
}

void ServerTile::setUnit(ServerUnit* unit, int layer) {
    units[layer] = unit;
}

int ServerTile::findUnit(const ServerUnit* unit ) const {
    for(int i=0;i<UNIT_LAYERS;i++) {
        if( units[i] == unit ) return i;
    }
    return -1;
}

int ServerTile::clearUnit(const ServerUnit* unit ) {
    int index = findUnit( unit );
    if( index >= 0 ) {
        units[ index ] = 0;
    }
    return index;
}

void ServerUnit::enterTile(ServerTile* newTile, int layer ) {
    using namespace std;
    if( tile ) {
        tile->clearUnit( this );
        tile = 0;
    }
    tile = newTile;
    tile->setUnit( this, layer );
}

int ServerUnit::leaveTile(void) {
    if( tile ) {
        return tile->clearUnit( this );
    }
    return -1;
}

bool ServerTile::mayEnter(const ServerUnit* unit) const {
    if( !tileType->mayTraverse( unit->getUnitType() ) ) return false;
    int layer = unit->getLayer();
    if( layer < 0 ) return false; // !?
    return units[layer] == 0;
}

int ServerUnit::getLayer(void) const {
    if( tile ) {
        int layer = tile->findUnit( this );
        return (layer >= 0) ? layer : unitType.nativeLayer;
    }
    return unitType.nativeLayer;
}

void ServerPlayer::removeControlledUnit(ServerUnit* unit) {
    std::vector<ServerUnit*>::iterator i = find( controlledUnits.begin(), controlledUnits.end(), unit );
    if( i != controlledUnits.end() ) {
        controlledUnits.erase( i );
    }
}

void ServerUnit::setController(ServerPlayer* player) {
    if( controller ) {
        controller->removeControlledUnit( this );
    }
    controller = player;
    controller->addControlledUnit( this );
}

ServerTile* ServerMap::getRandomTileFor(const ServerUnit* unit) {
    int tries = 1000;
    int mapIndexableSize = tiles.getSize();
    MTRand_int32 prng;
    
    while( tries-- > 0 ) {
        int guess = abs(prng()) % mapIndexableSize;
        ServerTile& tile = tiles.get( guess );
        using namespace std;
        if( tile.mayEnter( unit ) ) {
            int x, y;
            tile.getXY( x, y );
            using namespace std;
            cerr << "chose random tile " << x << " " << y << endl;
            return &tiles.get( guess );
        }
    }


    return 0;
}

ServerMap::~ServerMap(void) {
    for(std::map<int,ServerUnit*>::iterator i = units.begin(); i != units.end(); i++) {
        delete i->second;
    }
    for(std::map<int,ServerPlayer*>::iterator i = players.begin(); i != players.end(); i++) {
        delete i->second;
    }
}

void ServerMap::adoptPlayer(ServerPlayer* player) {
    std::map<int, ServerPlayer*>::iterator i = players.find( player->getId() );
    if( i != players.end() ) {
        throw std::runtime_error( "oops: reusing player id or readopting player" );
    }
    players[ player->getId() ] = player;
}

void ServerMap::adoptUnit(ServerUnit* unit) {
    std::map<int, ServerUnit*>::iterator i = units.find( unit->getId() );
    if( i != units.end() ) {
        throw std::runtime_error( "oops: reusing unit id or readopting unit" );
    }
    units[ unit->getId() ] = unit;
}

void ServerUnit::gatherFov( const ServerMap& smap, HexTools::HexFovRegion& region ) const {
    if( tile ) {
        int x, y;
        tile->getXY( x, y );
        using namespace std;
        HexTools::HexFov fov ( smap, region, x, y );
        fov.calculate();
    }
}

bool ServerMap::isOpaque(int x, int y) const {
    return tiles.get(x,y).getTileType().opacity == Type::BLOCK;
}

void ServerPlayer::gatherIndividualFov(const ServerMap& smap) {
    individualFov.clear();
    for(std::vector<ServerUnit*>::iterator i = controlledUnits.begin(); i != controlledUnits.end(); i++) {
        (*i)->gatherFov( smap, individualFov );
    }
}

ServerUnit* ServerMap::getUnitById(int id) {
    std::map<int,ServerUnit*>::iterator i = units.find( id );
    if( units.end() == i ) return 0;
    return i->second;
}

bool ServerMap::actionRemoveUnit(ServerUnit *unit) {
    ServerTile* tile = unit->getTile();
    unit->leaveTile();

    evtUnitDisappears( *unit, *tile );

    return true;
}

bool ServerMap::actionPlaceUnit(ServerUnit *unit, int x, int y ) {
    ServerTile& enteringTile = tiles.get( x, y );
    if( !enteringTile.mayEnter( unit ) ) return false;

    using namespace std;
    unit->enterTile( &enteringTile, unit->getLayer() );

    evtUnitAppears( *unit, enteringTile );

    return true;
}

bool ServerMap::actionMoveUnit(ServerUnit *unit, int dx, int dy) {
    if( !((abs(dx) == 3 && abs(dy) == 1)
          ||(dx == 0 && abs(dy) == 2)) ) return false;
    ServerTile *leavingTile = unit->getTile();
    if( !leavingTile ) return false;
    int x, y;
    leavingTile->getXY( x, y );
    ServerTile& enteringTile = tiles.get( x + dx, y + dy );
    if( !enteringTile.mayEnter( unit ) ) return false;

    // ok!
    unit->enterTile( &enteringTile, unit->getLayer() );

    // that was an event which might mean we need to send data to
    // several players
    evtUnitMoved( *unit, *leavingTile, enteringTile );

    return true;
}

bool ServerMap::cmdMoveUnit(ServerPlayer* player,int unitId, int dx, int dy) {
    ServerUnit *unit = getUnitById(unitId);
    if( !player || !unit ) return false;
    if( player != unit->getController() ) return false;
    return actionMoveUnit( unit, dx, dy );
}

void ServerPlayer::updateFov(const ServerMap& smap) {
    gatherIndividualFov( smap );
    // and share with allies!
}

bool ServerPlayer::isObserving(const ServerTile& tile) const {
    // or check allies here?
    int x, y;
    tile.getXY( x, y );
    return individualFov.contains( x, y );
}

void ServerMap::evtUnitAppears(ServerUnit& unit, ServerTile& tile) {
    for(std::map<int,ServerPlayer*>::iterator i = players.begin(); i != players.end(); i++) {
        ServerPlayer *player = i->second;
        if( player->isReceivingFovFrom( unit ) ) {
            player->updateFov( *this );
        }
    }
    for(std::map<int,ServerPlayer*>::iterator i = players.begin(); i != players.end(); i++) {
        ServerPlayer *player = i->second;
        if( player->isReceivingFovFrom( unit ) ) {
            player->sendFovDelta();
        }
        if( player->isObserving( tile ) ) {
            player->sendUnitDiscovered( unit );
        }
    }
}

void ServerMap::evtUnitDisappears(ServerUnit& unit, ServerTile& tile) {
    for(std::map<int,ServerPlayer*>::iterator i = players.begin(); i != players.end(); i++) {
        ServerPlayer *player = i->second;
        if( player->isReceivingFovFrom( unit ) ) {
            player->updateFov( *this );
        }
    }
    for(std::map<int,ServerPlayer*>::iterator i = players.begin(); i != players.end(); i++) {
        ServerPlayer *player = i->second;
        if( player->isReceivingFovFrom( unit ) ) {
            player->sendFovDelta();
        }
        if( player->isObserving( tile ) ) {
            player->sendUnitDisappears( unit );
        }
    }
}

void ServerMap::evtUnitMoved(ServerUnit& unit, ServerTile& sourceTile, ServerTile& destinationTile) {
    // what happened was that the unit _moved_ (different from:
    // appeared, disappeared, etc.)
    // this event is atomically observed, does that go for all reasonable
    // events? [you either see all of the event or none of it]
    // probably not, best not to assume
    // who will see this event?
    //   -everyone observing (having in the active set) either tile
    // what will they see?
    //   -discover the unit at the source tile, if not already discovered
    //   -observe the movement
    // actually, there's another dimension -- anyone receiving FOV from the unit will observe FOV deltas
    for(std::map<int,ServerPlayer*>::iterator i = players.begin(); i != players.end(); i++) {
        ServerPlayer *player = i->second;
        if( player->isReceivingFovFrom( unit ) ) {
            player->updateFov( *this );
        }
    }
    for(std::map<int,ServerPlayer*>::iterator i = players.begin(); i != players.end(); i++) {
        ServerPlayer *player = i->second;
        if( !player ) continue;
        bool observedSource = player->isObserving( sourceTile );
        bool observedDest = player->isObserving( destinationTile );
        if( player->isReceivingFovFrom( unit ) ) {
            player->sendFovDelta();
        }
        if( observedSource || observedDest ) {
            if( !observedSource ) {
                player->sendUnitDiscoveredAt( unit, sourceTile );
            }
            player->sendUnitMoved( unit, sourceTile, destinationTile );
            if( !observedDest ) {
                player->sendUnitDisappears( unit );
            }
        }
    }
}

void ServerPlayer::sendUnitDisappears(const ServerUnit& unit) {
    using namespace std;
    using namespace SProto;
    using namespace Sise;
    RemoteClient *rc = server.getConnectedUser( username );
    if( !rc ) return;
    rc->delsend( List()( new Symbol( "tac" ) )
                       ( new Symbol( "unit-disappears" ) )
                       ( new Int( unit.getId() ) )
                 .make() );
}

void ServerPlayer::sendUnitDiscovered(const ServerUnit& unit) {
    const ServerTile* tile = unit.getTile();
    if( tile ) {
        sendUnitDiscoveredAt( unit, *tile );
    }
}

void ServerPlayer::sendUnitDiscoveredAt(const ServerUnit& unit, const ServerTile& tile) {
    using namespace std;
    using namespace SProto;
    using namespace Sise;
    int x, y;
    int unitTeam = 0;
    const ServerPlayer* controller = unit.getController();
    RemoteClient *rc = server.getConnectedUser( username );
    if( !rc ) return;
    tile.getXY( x, y );
    rc->delsend( List()( new Symbol( "tac" ) )
                       ( new Symbol( "unit-discovered" ) )
                       ( new Int( unit.getId() ) )
                       ( new Symbol( unit.getUnitType().symbol ) )
                       ( new Int( controller ? controller->getId() : 0 ) )
                       ( new Int( unitTeam ) ) // hack: no teams yet xx
                       ( new Int( x ) )
                       ( new Int( y ) )
                       ( new Int( unit.getLayer() ) )
                 .make() );
}

void ServerPlayer::sendUnitMoved(const ServerUnit& unit, const ServerTile& fromTile, const ServerTile& toTile) {
    using namespace std;
    using namespace SProto;
    using namespace Sise;
    RemoteClient *rc = server.getConnectedUser( username );
    int x0, y0, x1, y1;
    if( !rc ) return;
    fromTile.getXY( x0, y0 );
    toTile.getXY( x1, y1 );
    rc->delsend( List()( new Symbol( "tac" ) )
                       ( new Symbol( "unit-moved" ) )
                       ( new Int( unit.getId() ) )
                       ( new Int( x1 - x0 ) )
                       ( new Int( y1 - y0 ) )
                 .make() );
}

TacTestServer::TacTestServer(SProto::Server& server, int radius) :
    SProto::SubServer( "tactest", server ),
    borderType( "border", "impassable wall", Type::WALL, Type::BLOCK, true, 0 ),
    wallType ( "wall", "wall", Type::WALL, Type::BLOCK, false, 0 ),
    floorType ( "floor", "floor", Type::FLOOR, Type::CLEAR, false, 100 ),
    pcType ( "pc", "Player" ),
    trollType ( "troll", "Troll" ),
    myMap ( radius, &borderType )
{
    trivialLevelGenerator( myMap, &wallType, &floorType, 0.4 );
}

bool TacTestServer::handle( SProto::RemoteClient* cli, const std::string& cmd, Sise::SExp *arg) {
    using namespace SProto;
    using namespace Sise;
    using namespace std;
    cerr << "handling " << cmd << " from " << cli->getUsername() << endl;
    ServerPlayer *player = 0;
    if( cli->hasUsername() ) {
        player = myMap.getPlayerByUsername( cli->getUsername() );
    }
    if( cmd == "test" ) {
        cli->delsend(( List()(new Symbol( "hello" ))
                              (new Symbol( "world" ))
                              (new String( cli->getUsername() ))
                        .make() ));
    } else if( cmd == "move-unit" ) {
        using namespace std;
        Cons *args = asProperCons( arg );
        if( !player ) return false;
        int unitId = *asInt( args->nthcar(0) );
        int dx = *asInt( args->nthcar(1) );
        int dy = *asInt( args->nthcar(2) );
        myMap.cmdMoveUnit( player, unitId, dx, dy );
    } else if( cmd == "test-spawn" ) {
        if( player ) {
            ServerUnit *unit = player->getAnyControlledUnit();
            player->sendMemories();
            player->assumeAmnesia();
            player->sendFovDelta();
            cli->delsend(( List()(new Symbol( "tactest" ))
                                 (new Symbol( "welcome" ))
                                 (new String( cli->getUsername() ))
                                 (new Int( unit->getId() ) )
                            .make() ));
            // no recovery of memories yet
            return true;
        }
        player = new ServerPlayer( server, myMap, myMap.generatePlayerId(), cli->getUsername() );
        ServerUnit *unit = new ServerUnit( myMap.generateUnitId(), pcType );
        ServerTile *tile = myMap.getRandomTileFor( unit );
        if( !tile ) {
            delete player;
            delete unit;
            return true;
        }
        myMap.adoptPlayer( player );
        myMap.adoptUnit( unit );
        unit->setController( player );
        int x, y;
        using namespace std;
        tile->getXY( x, y );
        myMap.actionPlaceUnit( unit, x, y );
        cli->delsend(( List()(new Symbol( "tactest" ))
                             (new Symbol( "welcome" ))
                             (new String( cli->getUsername() ))
                             (new Int( unit->getId() ) )
                        .make() ));
    } else {
        return false;
    }
    return true;
}

ServerPlayer* ServerMap::getPlayerByUsername(const std::string& username) {
    for(std::map<int,ServerPlayer*>::iterator i = players.begin(); i != players.end(); i++) {
        if( i->second->getUsername() == username ) {
            return i->second;
        }
    }
    return 0;
}

const HexTools::HexRegion& ServerPlayer::getTotalFov(void) {
    // filler
    return individualFov;
}

bool ServerPlayer::isReceivingFovFrom(const ServerUnit& unit) const {
    // no allies yet
    return unit.getController() == this;
}

void ServerPlayer::sendMemories(void) {
    using namespace HexTools;
    using namespace Sise;
    using namespace SProto;
    Cons *recall = 0;
    int sz = memory.getSize();
    for(int i=0;i<sz;i++) {
        int x, y;
        inflateHexCoordinate( i, x, y );
        const TileType *tt = memory.get(i);
        if( tt ) {
            recall = new Cons( List()( new Int( x ) )
                                     ( new Int( y ) )
                                     ( new Symbol( tt->symbol ) )
                               .make(),
                               recall );
        }
    }
    if( recall ) {
        RemoteClient *rc = server.getConnectedUser( username );
        if( !rc ) return;
        rc->delsend( new Cons( new Symbol( "tac" ),
                     new Cons( new Symbol( "terrain-discovered" ),
                               recall )));
        recall = 0;
    }
}

void ServerPlayer::sendFovDelta(void) {
    using namespace HexTools;
    using namespace Sise;
    using namespace SProto;

    const HexRegion& currentFov = getTotalFov();
    // this means: for newly bright areas,
    //             send notification of brightness
    //             plus IF NECESSARY (not remembered correctly)
    //              the tile type (updating memory)
    //             for newly dark areas, send notification of darkness


    Cons *newlyDark = 0, *newlyBright = 0;

    using namespace std;

    for(HexRegion::const_iterator i = transmittedActive.begin(); i != transmittedActive.end(); i++) {
        if( !currentFov.contains( i->first, i->second ) ) {
            newlyDark = new Cons( List()( new Int( i->first ) )
                                        ( new Int( i->second ) )
                                  .make(),
                                  newlyDark );
        }
    }

    for(HexRegion::const_iterator i = currentFov.begin(); i != currentFov.end(); i++) {
        if( !transmittedActive.contains( i->first, i->second ) ) {
            const ServerTile& tile = smap.getTile( i->first, i->second );
            const TileType *tt = &tile.getTileType();
            const TileType*& mem = memory.get( i->first, i->second );

            for(int j=0;j<UNIT_LAYERS;j++) {
                const ServerUnit * u = tile.getUnit(j);
                if( u ) {
                    sendUnitDiscovered( *u );
                }
            }

            if( tt != mem ) {
                newlyBright = new Cons( List()( new Int( i->first ) )
                                              ( new Int( i->second ) )
                                              ( new Symbol( tt->symbol ) )
                                        .make(),
                                        newlyBright );
                mem = tt;
            } else {
                newlyBright = new Cons( List()( new Int( i->first ) )
                                              ( new Int( i->second ) )
                                        .make(),
                                        newlyBright );
            }
        }
    }

    transmittedActive = currentFov;

    RemoteClient *rc = server.getConnectedUser( username );
    if( !rc ) {
        using namespace std;
        cerr << "warning: failed to get connected user (" << username << ")" << endl;
        return;
    }
    if( newlyBright ) {
        rc->delsend( new Cons( new Symbol( "tac" ),
                     new Cons( new Symbol( "fov-new-bright" ),
                               newlyBright )));
        newlyBright = 0;
    }
    if( newlyDark ) {
        rc->delsend( new Cons( new Symbol( "tac" ),
                     new Cons( new Symbol( "fov-new-dark" ),
                               newlyDark )));
        newlyDark = 0;
    }
}

};
