#include "TacServer.h"

#include "HexTools.h"

#include <iostream>

#include "BoxRandom.h"
#include "TacRules.h"

#include <algorithm>

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

ServerPlayer::ServerPlayer(SProto::Server& server, ServerMap& smap, int id, const std::string& username, ServerColour playerColour) :
    id ( id ),
    username ( username ),
    memory ( smap.getMapSize() ),
    server ( server ),
    smap ( smap ),
    playerColour ( playerColour )
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
    units (),
    gmpPrng( gmp_randinit_mt )
{
    reinitialize( defaultTt );
}

void ServerMap::reinitialize(TileType *defaultTt) {
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
    hp ( unitType.maxHp ),
    maxHp ( unitType.maxHp ),
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
        int rv = tile->clearUnit( this );
        tile = 0;
        return rv;
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
    if( controller ) {
        controller->addControlledUnit( this );
    }
}

ServerTile* ServerMap::getRandomTileForNear(const ServerUnit* unit, int cx, int cy) {
    const int tries = 1000;
    int i = 0;
    while( i < tries ) {
        int x, y;
        HexTools::inflateHexCoordinate( i, x, y );
        ServerTile& tile = tiles.get( x + cx, y + cy );
        if( tile.mayEnter( unit ) ) {
            return &tile;
        }
        i++;
    }
    return 0;
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

ServerPlayer* ServerMap::getPlayerById(int id) {
    std::map<int,ServerPlayer*>::iterator i = players.find( id );
    if( players.end() == i ) return 0;
    return i->second;
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

    unit->setController(0);

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

bool ServerMap::actionMeleeAttack(ServerUnit& attacker, ServerUnit& defender) {
    Outcomes<AttackResult> results = makeAttackBetween( *attacker.getUnitType().meleeAttack, defender.getUnitType().defense );
    AttackResult result = chooseRandomOutcome( results, gmpPrng );

    evtMeleeAttack( attacker, defender, result );
    
    return true;
}

bool ServerMap::cmdMeleeAttack(ServerPlayer* player,int unitId, int targetId ) {
    ServerUnit *unit = getUnitById( unitId );
    ServerUnit *target = getUnitById( targetId );
    if( !unit || !target ) return false;
    if( player != unit->getController() ) return false;

    if( !unit->getUnitType().meleeAttack ) return false;

    ServerTile *unitTile = unit->getTile();
    ServerTile *targetTile = target->getTile();
    if( !unitTile || !targetTile ) return false;
    int x0, y0, x1, y1;
    unitTile->getXY( x0, y0 );
    targetTile->getXY( x1, y1 );
    int dx = x1 - x0, dy = y1 - y0;
    if( !( (abs(dx) == 3 && abs(dy) == 1) || (dx == 0 && abs(dy) == 2) ) ) {
        return false;
    }

    int cost = 1;

    if( !unit->getAP().maySpendActionPoints( cost ) ) return false;

    bool rv = actionMeleeAttack( *unit, *target );

    if( rv ) {
        unit->getAP().spendActionPoint( cost );
        unit->getAP().forbidMovement(); // no hit and run rule

        evtUnitActivityChanged( *unit );
    }

    
    return rv;
}

bool ServerMap::cmdMoveUnit(ServerPlayer* player,int unitId, int dx, int dy) {
    ServerUnit *unit = getUnitById(unitId);
    if( !player || !unit ) return false;
    if( player != unit->getController() ) return false;

    // check energy
    ServerTile *leavingTile = unit->getTile();
    if( !leavingTile ) return false;
    int x, y;
    leavingTile->getXY( x, y );
    ServerTile& enteringTile = tiles.get( x + dx, y + dy );
    mpq_class cost;
    if( !enteringTile.getTileType().mayTraverse( unit->getUnitType(), cost ) ) return false;
    if( !unit->getAP().maySpendMovementEnergy( cost ) ) return false;

    bool rv = actionMoveUnit( unit, dx, dy );

    if( rv ) {
        unit->getAP().spendMovementEnergy( cost );

        evtUnitActivityChanged( *unit );
    }
    
    return rv;
}

void ServerPlayer::updateFov(const ServerMap& smap) {
    gatherIndividualFov( smap );
    // and share with allies!
}

bool ServerPlayer::isObserving(const ServerUnit& unit) const {
    const ServerTile *tile = unit.getTile();
    if( !tile ) return false;
    return isObserving( *tile );
}

bool ServerPlayer::isObserving(const ServerTile& tile) const {
    // or check allies here?
    int x, y;
    tile.getXY( x, y );
    return individualFov.contains( x, y );
}

void ServerMap::evtUnitActivityChanged(ServerUnit& unit) {
    using namespace std;
    for(std::map<int,ServerPlayer*>::iterator i = players.begin(); i != players.end(); i++) {
        ServerPlayer *player = i->second;
        if( player->isObserving( unit ) ) {
            player->sendUnitAP( unit );
        }
    }
}

void ServerMap::evtMeleeAttack(ServerUnit& attacker, ServerUnit& defender, AttackResult result) {
    for(std::map<int,ServerPlayer*>::iterator i = players.begin(); i != players.end(); i++) {
        ServerPlayer *player = i->second;
        if( player->isObserving( attacker ) || player->isObserving( defender ) ) {
            player->sendMeleeAttack( attacker, defender, result );
        }
    }

    defender.applyAttack( result );

    if( defender.isDead() ) {
        actionRemoveUnit( &defender );
    }
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

void ServerPlayer::sendPlayerTurnBegins(const ServerPlayer& player, double timeLeft) {
    using namespace std;
    using namespace SProto;
    using namespace Sise;
    RemoteClient *rc = server.getConnectedUser( username );
    if( !rc ) return;
    rc->delsend( List()( new Symbol( "tac" ) )
                       ( new Symbol( "player-turn-begins" ) )
                       ( new Int( player.getId() ) )
                       ( new Int( (int)(0.5 + 1000 * timeLeft) ) )
                 .make() );
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

void ServerPlayer::sendUnitAP(const ServerUnit& unit) {
    using namespace Sise;
    using namespace SProto;
    RemoteClient *rc = server.getConnectedUser( username );
    if( !rc ) return;
    rc->delsend( List()( new Symbol( "tac" ) )
                       ( new Symbol( "ap-update" ) )
                       ( new Int( unit.getId() ) )
                       ( unit.getAP().toSexp() )
                 .make() );
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
                       ( new Int( unitTeam ) ) // hack: no teams yet xx
                       ( new Int( controller ? controller->getId() : 0 ) )
                       ( new Int( x ) )
                       ( new Int( y ) )
                       ( new Int( unit.getLayer() ) )
                       ( unit.getAP().toSexp() )
                       ( new Int( unit.getHP() ) )
                       ( new Int( unit.getMaxHP() ) )
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

TacTestServer::TacTestServer(SProto::Server& server, int radius, const std::string& unitsfn, const std::string& tilesfn) :
    SProto::SubServer( "tactest", server ),
    myMap ( radius, 0 )
{
    fillManagerFromFile( unitsfn, unitTypes );
    fillManagerFromFile( tilesfn, tileTypes );
    myMap.reinitialize( &tileTypes[ "border" ] );
    trivialLevelGenerator( myMap, &tileTypes[ "std-wall" ], &tileTypes[ "std-floor" ], 0.4 );

    // fill this from a lisp file? or is that overkill?
    colourPool.add( 255, 0, 0 );
    colourPool.add( 0, 255, 0 );
    colourPool.add( 0, 0, 255 );
    colourPool.add( 255, 255, 0 );
    colourPool.add( 255, 0, 255 );
    colourPool.add( 0, 255, 255 );

}

void TacTestServer::delbroadcast(Sise::SExp* sexp) {
    using namespace SProto;
    using namespace Sise;
    using namespace std;
    for(std::set<std::string>::iterator i = clients.begin(); i != clients.end(); i++) {
        RemoteClient *cli = server.getConnectedUser( *i );
        if( !cli ) continue;
        cli->send( sexp );
    }
    delete sexp;
}

void TacTestServer::tick(double dt) {
    if( turns.getNumberOfParticipants() == 0) return;
    using namespace std;
    if( turns.getCurrentRemainingTime() <= 0 ) {
        turns.next();
        announceTurn();
    }
}

bool TacTestServer::hasTurn(ServerPlayer* player) {
    return player->getId() == turns.current();
}

void TacTestServer::announceTurn(void) {
    using namespace SProto;
    using namespace Sise;
    using namespace std;
    int playerId = turns.current();
    int nopa = turns.getNumberOfParticipants();
    while( !myMap.getPlayerById( playerId )
           || !server.getConnectedUser( myMap.getPlayerById( playerId )->getUsername() )
           || myMap.getPlayerById( playerId )->getNumberOfUnits() == 0 ) {
        if( playerId == -1 ) return;
        if( !myMap.getPlayerById( playerId ) ) {
            turns.removeParticipant( playerId );
            playerId = turns.current();
        } else {
            playerId = turns.skip();
        }
        --nopa;
        if( nopa <= 0 ) return;
    }
    ostringstream oss;
    oss << "*** ";
    oss << "To move: " << myMap.getPlayerById( playerId )->getUsername();
    oss << " in " << formatTime( turns.getCurrentRemainingTime() );
    myMap.actionPlayerTurnBegins( *myMap.getPlayerById( playerId ), turns.getCurrentRemainingTime() );
    using namespace std;
    delbroadcast( new Cons( new Symbol( "chat" ),
                  new Cons( new Symbol( "channel" ),
                  new Cons( new Symbol( "tactest" ), 
                  new Cons( new String( "tactest" ),
                            prepareChatMessage( "", oss.str() ))))));

}

void ServerMap::actionPlayerTurnBegins(ServerPlayer& turnPlayer, double timeLeft) {
    turnPlayer.beginTurn();

    for(std::map<int,ServerPlayer*>::iterator i = players.begin(); i != players.end(); i++) {
        ServerPlayer *player = i->second;
        player->sendPlayerTurnBegins( turnPlayer, timeLeft );
    }
}

bool TacTestServer::handle( SProto::RemoteClient* cli, const std::string& cmd, Sise::SExp *arg) {
    using namespace SProto;
    using namespace Sise;
    using namespace std;
    ServerPlayer *player = 0;
    if( cli->hasUsername() ) {
        player = myMap.getPlayerByUsername( cli->getUsername() );
    }
    if( cmd == "test" ) {
        cli->delsend(( List()(new Symbol( "hello" ))
                              (new Symbol( "world" ))
                              (new String( cli->getUsername() ))
                        .make() ));
    } else if( cmd == "pass" ) {
        if( !player ) return false;
        if( !hasTurn(player) ) return false;
        turns.next();
        announceTurn();
    } else if( cmd == "melee-attack" ) {
        using namespace std;
        Cons *args = asProperCons( arg );
        if( !player ) return false;
        if( !hasTurn(player) ) return false;
        int unitId = *asInt( args->nthcar(0) );
        int targetUnitId = *asInt( args->nthcar(1) );
        myMap.cmdMeleeAttack( player, unitId, targetUnitId );
    } else if( cmd == "move-unit" ) {
        using namespace std;
        Cons *args = asProperCons( arg );
        if( !player ) return false;
        if( !hasTurn(player) ) return false;
        int unitId = *asInt( args->nthcar(0) );
        int dx = *asInt( args->nthcar(1) );
        int dy = *asInt( args->nthcar(2) );
        myMap.cmdMoveUnit( player, unitId, dx, dy );
    } else if( cmd == "test-spawn" ) {
        cli->enterChannel( "tactest", "tactest" );
        clients.insert( cli->getUsername() );
        if( player ) {
            myMap.actionNewPlayer(*player);
            player->sendMemories();
            player->assumeAmnesia();
            player->sendFovDelta();
            ServerPlayer *currentPlayer = myMap.getPlayerById( turns.current() );
            if( currentPlayer ) {
                player->sendPlayerTurnBegins( *currentPlayer, turns.getCurrentRemainingTime() );
            }
            ServerUnit *unit = player->getAnyControlledUnit();
            if( !unit ) {
                spawnPlayerUnits( player );
                unit = player->getAnyControlledUnit();
            }
            cli->delsend(( List()(new Symbol( "tactest" ))
                                 (new Symbol( "welcome" ))
                                 (new String( cli->getUsername() ))
                                 (new Int( unit->getId() ) )
                            .make() ));
//            turns.addParticipant( player->getId(), 10.0, 10.0 );
            return true;
        }
        player = new ServerPlayer( server, myMap, myMap.generatePlayerId(), cli->getUsername(), colourPool.next() );
        myMap.adoptPlayer( player );
        myMap.actionNewPlayer(*player);
        spawnPlayerUnits( player );

        turns.addParticipant( player->getId(), 30.0, 30.0 );
        using namespace std;
        if( turns.getNumberOfParticipants() == 1 ) {
            turns.start();
            announceTurn();
        } else {
            ServerPlayer *currentPlayer = myMap.getPlayerById( turns.current() );
            if( currentPlayer ) {
                player->sendPlayerTurnBegins( *currentPlayer, turns.getCurrentRemainingTime() );
            }
        }
        cli->delsend(( List()(new Symbol( "tactest" ))
                             (new Symbol( "welcome" ))
                             (new String( cli->getUsername() ))
                             (new Int( player->getAnyControlledUnit()->getId() ) )
                        .make() ));
    } else {
        return false;
    }

    checkWinLossCondition();

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

void ServerUnit::beginTurn(void) {
    activity = ActivityPoints(unitType, 1, 1, 1);
}

void ServerPlayer::beginTurn(void) {
    for(std::vector<ServerUnit*>::iterator i = controlledUnits.begin(); i != controlledUnits.end(); i++) {
        (*i)->beginTurn();
    }
}

void ServerUnit::applyAttack(AttackResult result) {
    if( result.status == AttackResult::HIT ) {
        hp -= result.damage;
    }
}

bool ServerUnit::isDead(void) const {
    return hp <= 0;
}

void ServerMap::actionNewPlayer(ServerPlayer& player) {
    for(std::map<int,ServerPlayer*>::iterator i = players.begin(); i != players.end(); i++) {
        player.sendPlayer( *i->second );
        i->second->sendPlayer( player );
    }
}

void ServerPlayer::sendPlayer(const ServerPlayer& player) {
    using namespace Sise;
    using namespace SProto;
    using namespace HexTools;
    RemoteClient *rc = server.getConnectedUser( username );
    if( !rc ) return;
    rc->delsend( List()( new Symbol( "tac" ) )
                       ( new Symbol( "introduce-player" ) )
                       ( new Int( player.getId() ) )
                       ( new String( player.getUsername() ) )
                       ( new String( player.getUsername() ) )
                       ( player.getColour().toSexp() )
                 .make() );
}

void ServerPlayer::sendMeleeAttack(const ServerUnit& attacker, const ServerUnit& defender, AttackResult result) {
    if( !isObserving( attacker ) ) {
        sendUnitDiscovered( attacker );
    }
    if( !isObserving( defender ) ) {
        sendUnitDiscovered( defender );
    }
    using namespace Sise;
    using namespace SProto;
    using namespace HexTools;
    RemoteClient *rc = server.getConnectedUser( username );
    if( !rc ) return;
    rc->delsend( List()( new Symbol( "tac" ) )
                       ( new Symbol( "melee-attack" ) )
                       ( new Int( attacker.getId() ) )
                       ( new Int( defender.getId() ) )
                       ( result.toSexp() )
                 .make() );

}

void TacTestServer::spawnPlayerUnits(ServerPlayer* player) {
    using namespace std;
    ServerUnit *unit1, *unit2, *unit3;
    unit1 = new ServerUnit( myMap.generateUnitId(), unitTypes["scout"] );

    int x, y;
    ServerTile *tile1 = myMap.getRandomTileFor( unit1 );
    assert( tile1 );
    myMap.adoptUnit( unit1 );
    unit1->setController( player );
    unit1->getAP() = ActivityPoints( unit1->getUnitType(), 1, 1, 1 );
    tile1->getXY( x, y );
    myMap.actionPlaceUnit( unit1, x, y );

    unit2 = new ServerUnit( myMap.generateUnitId(), unitTypes["swordsman"] );
    ServerTile *tile2 = myMap.getRandomTileForNear( unit2, x, y );
    assert( tile2 );
    tile2->getXY( x, y );
    myMap.adoptUnit( unit2 );
    unit2->setController( player );
    unit2->getAP() = ActivityPoints( unit2->getUnitType(), 1, 1, 1 );
    myMap.actionPlaceUnit( unit2, x, y );

    unit3 = new ServerUnit( myMap.generateUnitId(), unitTypes["shieldmaiden"] );
    ServerTile *tile3 = myMap.getRandomTileForNear( unit3, x, y );
    assert( tile3 );
    tile3->getXY( x, y );
    myMap.adoptUnit( unit3 );
    unit3->setController( player );
    unit3->getAP() = ActivityPoints( unit3->getUnitType(), 1, 1, 1 );
    myMap.actionPlaceUnit( unit3, x, y );
}

void TacTestServer::checkWinLossCondition(void) {
    using namespace Sise;
    using namespace SProto;
    for(std::set<std::string>::iterator i = clients.begin(); i != clients.end(); i++) {
        std::string username = *i;
        ServerPlayer *player = myMap.getPlayerByUsername( username );
        if( !player ) continue;
        if( find( defeatedPlayers.begin(), defeatedPlayers.end(), username ) != defeatedPlayers.end() ) continue;
        if( player->getNumberOfUnits() == 0 ) {
            defeatedPlayers.insert( username );
            std::ostringstream oss;
            oss << "*** ";
            oss << "Player " << username << " has been defeated!";
            delbroadcast( new Cons( new Symbol( "chat" ),
                          new Cons( new Symbol( "channel" ),
                          new Cons( new Symbol( "tactest" ), 
                          new Cons( new String( "tactest" ),
                                    prepareChatMessage( "", oss.str() ))))));
        }
    }
}

Sise::SExp* ServerColour::toSexp(void) const {
    using namespace Sise;
    using namespace SProto;
    return List()( new Int( r ) )
                 ( new Int( g ) )
                 ( new Int( b ) )
           .make();
}

};
