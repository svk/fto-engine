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

RememberedInformation::RememberedInformation(int mapSize) :
    memory ( mapSize )
{
    int sz = memory.getSize();
    memory.getDefault() = 0;
    for(int i=0;i<sz;i++) {
        memory.get(i) = 0;
    }
}

ServerPlayer::ServerPlayer(int id, const std::string& username, const ServerMap& smap) :
    id ( id ),
    username ( username ),
    memory ( smap.getMapSize() )
{
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
    for(int r=0;r<=mapSize;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++) {
        int x, y;
        HexTools::cartesianiseHexCoordinate( i, j, r, x, y );
        tiles.get(x,y).setXY(x,y);
        tiles.get(x,y).setTileType( defaultTt );
    }
    tiles.getDefault().setTileType( defaultTt );
}

ServerUnit::ServerUnit(int id, const UnitType& unitType) :
    id ( id ),
    unitType ( unitType ),
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

void ServerUnit::enterTile(ServerTile* tile, int layer ) {
    if( tile ) {
        tile->clearUnit( this );
        tile = 0;
    }
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
    MTRand prng;

    while( tries-- > 0 ) {
        int guess = abs(prng()) % mapIndexableSize;
        if( tiles.get( guess ).mayEnter( unit ) ) {
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
        HexTools::HexFov fov ( smap, region, x, y );
        fov.calculate();
    }
}

bool ServerMap::isOpaque(int x, int y) const {
    return tiles.get(x,y).getTileType().opacity == Type::CLEAR;
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
    unit->leaveTile();
    return true;
}

bool ServerMap::actionPlaceUnit(ServerUnit *unit, int x, int y ) {
    ServerTile& enteringTile = tiles.get( x, y );
    if( !enteringTile.mayEnter( unit ) ) return false;

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
        if( player->isObserving( tile ) ) {
            player->sendUnitDiscovered( unit );
        }
    }
}

void ServerMap::evtUnitDisappears(ServerUnit& unit, ServerTile& tile) {
    for(std::map<int,ServerPlayer*>::iterator i = players.begin(); i != players.end(); i++) {
        ServerPlayer *player = i->second;
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
    for(std::map<int,ServerPlayer*>::iterator i = players.begin(); i != players.end(); i++) {
        ServerPlayer *player = i->second;
        bool observedSource = player->isObserving( sourceTile );
        if( observedSource || player->isObserving( destinationTile ) ) {
            if( !observedSource ) {
                player->sendUnitDiscovered( unit );
            }
            player->sendUnitMoved( unit, sourceTile, destinationTile );
        }
    }
}

void ServerPlayer::sendUnitDisappears(const ServerUnit& unit) {
    using namespace std;
    cerr << "would send unit discovered" << endl;
}

void ServerPlayer::sendUnitDiscovered(const ServerUnit& unit) {
    using namespace std;
    cerr << "would send unit discovered" << endl;
}

void ServerPlayer::sendUnitMoved(const ServerUnit& unit, const ServerTile& fromTile, const ServerTile& toTile) {
    using namespace std;
    cerr << "would send unit moved" << endl;
}

TacTestServer::TacTestServer(SProto::Server& server, int radius) :
    SProto::SubServer( "tactest", server ),
    borderType( "impassable wall", Type::WALL, Type::BLOCK, true, 0 ),
    wallType ( "wall", Type::WALL, Type::BLOCK, false, 0 ),
    floorType ( "floor", Type::FLOOR, Type::CLEAR, false, 100 ),
    myMap ( radius, &borderType )
{
    trivialLevelGenerator( myMap, &wallType, &floorType, 0.4 );
}

bool TacTestServer::handle( SProto::RemoteClient* cli, const std::string& cmd, Sise::SExp *arg) {
    using namespace SProto;
    using namespace Sise;
    if( cmd == "test" ) {
        cli->delsend(( List()(new Symbol( "hello" ))
                              (new Symbol( "world" ))
                              (new String( cli->getUsername() ))
                        .make() ));
    } else {
        return false;
    }
    return true;
}

};
