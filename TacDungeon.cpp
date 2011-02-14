#include "TacDungeon.h"

namespace Tac {

DungeonSketch::DungeonSketch(int seed) :
    sketch ( ST_NONE ),
    prng ( seed )
{
}

DungeonSketch::~DungeonSketch(void) {
}

bool DungeonSketch::checkRoomAt(int cx, int cy, int R) const {
    using namespace HexTools;
    if( sketch.get(cx,cy) != ST_NONE ) return false;
    for(int r=1;r<=R;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++){
        int dx, dy;
        cartesianiseHexCoordinate( i, j, r, dx, dy );
        if( sketch.get(cx+dx,cy+dy) != ST_NONE ) return false;
    }
    return true;
}

void DungeonSketch::put(int x,int y, SketchTile st ){
    sketch.set( x, y, st );
}

HexTools::HexCoordinate DungeonSketch::placeRoomNear(int cx, int cy, int radius){
    using namespace HexTools;
    std::vector<HexCoordinate> rv;

    if( checkRoomAt( cx, cy, radius ) ) return HexCoordinate(cx,cy);

    for(int r=1;true;r++) {
        for(int i=0;i<6;i++) for(int j=0;j<r;j++){
            int dx, dy;
            cartesianiseHexCoordinate( i, j, r, dx, dy );
            if( checkRoomAt( cx + dx, cy + dy, radius ) ) {
                rv.push_back( HexCoordinate(cx + dx, cy + dy) );
            }
        }
        if( !rv.empty() ) break;
    }

    return rv[ abs( prng() ) % (int) rv.size() ];
}

RoomPainter::RoomPainter(int radius) :
    radius( radius )
{
}


HexagonRoomPainter::HexagonRoomPainter(int radius) :
    RoomPainter( radius )
{
}

void HexagonRoomPainter::paint(DungeonSketch& sketch, int cx, int cy) {
    using namespace HexTools;
    int R = getRadius();
    sketch.put(cx,cy, DungeonSketch::ST_NORMAL_FLOOR );
    for(int r=1;r<=R;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++){
        int x, y;
        cartesianiseHexCoordinate( i, j, r, x, y );
        if( r == R ) {
            sketch.put( cx + x, cy + y, DungeonSketch::ST_NORMAL_WALL );
        } else {
            sketch.put( cx + x, cy + y, DungeonSketch::ST_NORMAL_FLOOR );
        }
    }
}

HexTools::HexCoordinate DungeonSketch::paintRoomNear(RoomPainter& room, int cx, int cy) {
    using namespace HexTools;
    HexCoordinate rv = placeRoomNear(cx, cy, room.getRadius() );
    room.paint( *this, rv.first, rv.second );
    return rv;
}

}
