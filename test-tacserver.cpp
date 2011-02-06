#include "TacServer.h"

int main(int argc, char *argv[]) {
    using namespace Tac;
    TileType borderType( "impassable wall", Type::WALL, Type::BLOCK, true, 0 );
    ServerMap smap(40, &borderType);
    return 0;
}
