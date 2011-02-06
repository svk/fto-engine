#include "TacServer.h"

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
        unit[i] = 0;
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

ServerUnit::ServerUnit(int id, const UnitType& unitType, ServerPlayer *controller) :
    id ( id ),
    unitType ( unitType ),
    tile ( 0 ),
    controller ( controller )
{
}


};
