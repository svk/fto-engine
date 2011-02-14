#ifndef H_TAC_DUNGEON
#define H_TAC_DUNGEON

#include <vector>

#include "HexTools.h"

/* Basic shapes:
    - the hexagon (duh), oriented contra the hex tiles
        6 walls: e, ne, nw, w, sw, se
    - the rectangle -- fill in the top and bottom with "straight lines" (e.g. +1 on odd cols)
        4 walls: e, n, w, s
    - the diamond -- hexagon with no extent on the east/west walls
        4 walls: ne, nw, sw, se
*/

/* Metadata we need:
     ZOI (playerno#) - spawn zones
*/

#include "mtrand.h"

namespace Tac {

class RoomPainter;

class DungeonSketch {
    public:
        enum SketchTile {
            ST_NONE,
            ST_NORMAL_FLOOR,
            ST_NORMAL_WALL,
            ST_NORMAL_CORRIDOR,
            ST_NORMAL_DOORWAY,
            ST_META_DIGGABLE,
            ST_META_CONNECTOR
        };

    private:
        HexTools::SparseHexMap<SketchTile> sketch;
        MTRand_int32 prng;

    public:
        DungeonSketch(int);
        ~DungeonSketch(void);

        int getMaxRadius(void) const { return sketch.getMaxRadius(); }

        void put(int,int, SketchTile);
        SketchTile get(int x, int y) const { return sketch.get(x,y); }

        bool checkRoomAt(int,int,int) const;

        HexTools::HexCoordinate placeRoomNear(int,int,int);

        HexTools::HexCoordinate paintRoomNear(RoomPainter&,int,int);
};

class PointCorridor {
    private:
        DungeonSketch& sketch;
        int cx, cy;

        bool checked;
        bool activeDir[6];

        int length;

        static int dx[6], dy[6];

    public:
        PointCorridor(DungeonSketch&, int, int);

        bool check(void);
        void dig(void);
};

class RoomPainter {
    private:
        int radius;

    public:
        RoomPainter(int);

        void paint(int,int, DungeonSketch::SketchTile);
        int getRadius(void) const { return radius; }

        virtual void paint(DungeonSketch&,int,int) = 0;
};

class BlankRoomPainter : public RoomPainter {
    public:
        BlankRoomPainter(int);

        void paint(DungeonSketch&,int,int);
};

class HexagonRoomPainter : public RoomPainter {
    public:
        HexagonRoomPainter(int);

        void paint(DungeonSketch&,int,int);
};

class RectangularRoomPainter : public RoomPainter {
    public:
        RectangularRoomPainter(int);

        void paint(DungeonSketch&,int,int);
};

class HollowHexagonRoomPainter : public RoomPainter {
    private:
        int thickness;
    public:
        HollowHexagonRoomPainter(int, int);

        void paint(DungeonSketch&,int,int);
};

};

#endif
