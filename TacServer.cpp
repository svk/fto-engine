#include "TacServer.h"

#include "HexTools.h"

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
    players (),
    unitIdGen (),
    playerIdGen (),
    tiles ( mapSize )
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

};
