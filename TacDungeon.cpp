#include "TacDungeon.h"

namespace Tac {

int PointCorridor::dx[6] ={ 3, 0, -3, -3, 0, 3};
int PointCorridor::dy[6] ={ 1, 2, 1, -1, -2, -1};

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


BlankRoomPainter::BlankRoomPainter(int radius) :
    RoomPainter( radius )
{
}

RectangularRoomPainter::RectangularRoomPainter(int radius) :
    RoomPainter( radius )
{
    if( radius < 4 ) throw std::logic_error( "rectangular room too small" );
}


HollowHexagonRoomPainter::HollowHexagonRoomPainter(int radius, int thickness) :
    RoomPainter( radius ),
    thickness ( thickness )
{
    if( radius < (3 + thickness) ) throw std::logic_error( "hollow hexagon room too small" );
}


HexagonRoomPainter::HexagonRoomPainter(int radius) :
    RoomPainter( radius )
{
    if( radius < 4 ) throw std::logic_error( "hexagon room too small" );
}

void RectangularRoomPainter::paint(DungeonSketch& sketch, int cx, int cy) {
    using namespace HexTools;
    int R = getRadius();
    using namespace std;
    sketch.put( cx, cy, DungeonSketch::ST_META_DIGGABLE );
    for(int r=1;r<=getRadius();r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++){
        int x, y;
        cartesianiseHexCoordinate( i, j, r, x, y );
        sketch.put( cx + x, cy + y, DungeonSketch::ST_META_DIGGABLE );
    }
    R--;
    for(int i=-(R-1);i<=(R-1);i++) for(int j=-(R-1);j<(R-1);j++) { // don't touch this
        int x = cx + 3 * i, y = cy + 2 * j;
        int ci, cj, cr;
        if( (i%2) != 0 ) y++;
        HexTools::polariseHexCoordinate( x - cx, y - cy, ci, cj, cr );
        assert( cr <= getRadius() );
        if( abs(i) == (R-1) || abs(j) == (R-1) ) {
            if( !i || !j ) {
                sketch.put( cx + x, cy + y, DungeonSketch::ST_META_CONNECTOR );
            } else {
                sketch.put( cx + x, cy + y, DungeonSketch::ST_NORMAL_WALL );
            }
        } else{
            sketch.put( cx + x, cy + y, DungeonSketch::ST_NORMAL_FLOOR );
        }
    }
}

void BlankRoomPainter::paint(DungeonSketch& sketch, int cx, int cy) {
    using namespace HexTools;
    int R = getRadius();
    sketch.put(cx,cy, DungeonSketch::ST_META_DIGGABLE );
    for(int r=1;r<=R;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++){
        int x, y;
        cartesianiseHexCoordinate( i, j, r, x, y );
        sketch.put( cx + x, cy + y, DungeonSketch::ST_META_DIGGABLE );
    }
}

void HollowHexagonRoomPainter::paint(DungeonSketch& sketch, int cx, int cy) {
    using namespace HexTools;
    int R = getRadius();
    sketch.put(cx,cy, DungeonSketch::ST_NORMAL_WALL );
    for(int r=1;r<=R;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++){
        int x, y;
        cartesianiseHexCoordinate( i, j, r, x, y );
        if( r == R ) {
            sketch.put( cx + x, cy + y, DungeonSketch::ST_META_DIGGABLE );
        } else if( r == (R-1) ) {
            if( j == (R-1)/2 ) {
                sketch.put( cx + x, cy + y, DungeonSketch::ST_META_CONNECTOR );
            } else {
                sketch.put( cx + x, cy + y, DungeonSketch::ST_NORMAL_WALL );
            }
        } else if( r < ((R-1)-thickness)) {
            sketch.put( cx + x, cy + y, DungeonSketch::ST_NORMAL_WALL );
        } else {
            sketch.put( cx + x, cy + y, DungeonSketch::ST_NORMAL_FLOOR );
        }
    }
}

void HexagonRoomPainter::paint(DungeonSketch& sketch, int cx, int cy) {
    using namespace HexTools;
    int R = getRadius();
    sketch.put(cx,cy, DungeonSketch::ST_NORMAL_FLOOR );
    for(int r=1;r<=R;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++){
        int x, y;
        cartesianiseHexCoordinate( i, j, r, x, y );
        if( r == R ) {
            sketch.put( cx + x, cy + y, DungeonSketch::ST_META_DIGGABLE );
        } else if( r == (R-1) ) {
            if( j == (R-1)/2 ) {
                sketch.put( cx + x, cy + y, DungeonSketch::ST_META_CONNECTOR );
            } else {
                sketch.put( cx + x, cy + y, DungeonSketch::ST_NORMAL_WALL );
            }
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

PointCorridor::PointCorridor(DungeonSketch& sketch, int cx, int cy) :
    sketch ( sketch ),
    cx ( cx ),
    cy ( cy ),
    checked ( false )
{
    for(int i=0;i<6;i++) {
        activeDir[i] = false;
    }
}

bool PointCorridor::check(void) {
    int n = 0;
    const int lengthLimit = 30;
    length = 0;
    DungeonSketch::SketchTile mptt = sketch.get(cx,cy);
    if( mptt != DungeonSketch::ST_NONE && mptt != DungeonSketch::ST_META_DIGGABLE ) return false;
    for(int i=0;i<6;i++) {
        int tries = sketch.getMaxRadius() * 2 + 1;
        int x = cx, y = cy;
        int llen = 0;
        while( tries-- > 0 ) {
            DungeonSketch::SketchTile tt = sketch.get(x,y);
            if( tt != DungeonSketch::ST_NONE && tt != DungeonSketch::ST_META_DIGGABLE ) break;
            x += dx[i];
            y += dy[i];
            ++llen;
        }
        using namespace std;
        switch( sketch.get(x,y) ) {
            case DungeonSketch::ST_META_CONNECTOR:
                ++n;
                length += llen;
                activeDir[i] = true;
                break;
            default:
                activeDir[i] = false;
                break;
        }
    }
    checked = true;
    using namespace std;
    return n > 1 && (length < lengthLimit);
}

void PointCorridor::dig(void) {
    if( !checked ) {
        check();
    }
    using namespace std;
    for(int i=0;i<6;i++) if(activeDir[i] ){
        int x = cx, y = cy;
        while( sketch.get(x,y) != DungeonSketch::ST_META_CONNECTOR ) {
            for(int k=0;k<6;k++) {
                int sx = x + dx[k], sy = y + dy[k];
                if( sketch.get(sx, sy) == DungeonSketch::ST_NONE
                    || sketch.get(sx, sy) == DungeonSketch::ST_META_DIGGABLE ) {
                    sketch.put( sx, sy, DungeonSketch::ST_NORMAL_WALL );
                }
            }
            sketch.put(x, y, DungeonSketch::ST_NORMAL_CORRIDOR );
            using namespace std;
            x += dx[i];
            y += dy[i];
        }
        sketch.put( x, y, DungeonSketch::ST_NORMAL_DOORWAY );
    }
}

}
