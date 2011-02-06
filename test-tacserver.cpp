#include "TacServer.h"

int main(int argc, char *argv[]) {
    using namespace Tac;
    TileType borderType ( "impassable wall", Type::WALL, Type::BLOCK, true, 0 );
    TileType wallType ( "wall", Type::WALL, Type::BLOCK, false, 0 );
    TileType floorType ( "floor", Type::FLOOR, Type::CLEAR, false, 0 );

    ServerMap smap(40, &borderType);

    trivialLevelGenerator( smap, &wallType, &floorType, 0.4 );

    return 0;
}
