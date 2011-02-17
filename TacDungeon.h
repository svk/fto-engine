#ifndef H_TAC_DUNGEON
#define H_TAC_DUNGEON

#include <vector>

#include <list>
#include <set>

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

class RoomNode {
    private:
        bool connected;
        std::set<RoomNode*> connections;
        HexTools::HexRegion region; // floorspace -- spawn region

    public:
        RoomNode(void);

        void connect(RoomNode*);
        void add(int,int);
        void remove(int,int);

        bool isDirectlyConnected(RoomNode*) const;

        void markConnected(void);
        int getRegionSize(void) const;
        bool isMarkedConnected(void) const;
        void clearConnectedMark(void) { connected = false; }
};

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
        std::vector<RoomNode*> rooms;
        std::map<HexTools::HexCoordinate, RoomNode*> rConnectors;

    public:
        DungeonSketch(void);
        ~DungeonSketch(void);

        void registerConnector(int, int, RoomNode*);
        RoomNode* getConnectorRoom(int, int);

        void adoptRoom(RoomNode*);

        int getMaxRadius(void) const { return sketch.getMaxRadius(); }

        void put(int,int, SketchTile);
        SketchTile get(int x, int y) const { return sketch.get(x,y); }

        bool checkRoomAt(int,int,int) const;

        std::vector<RoomNode*>& getRooms(void) { return rooms; }
};

class PointCorridor {
    private:
        DungeonSketch* sketch;
        int cx, cy;

        bool checked;
        bool activeDir[6];
        RoomNode* node[6];

        int length;

        static int dx[6], dy[6];

    public:
        PointCorridor(DungeonSketch&, int, int);
        PointCorridor(const PointCorridor&);

        const PointCorridor& operator=(const PointCorridor&);

        bool check(void);
        void dig(void);

        int getLength(void) const { return length; }

        bool isActive(int j) const { return activeDir[j]; }
        RoomNode* getNode(int j) { return node[j]; }
};

class RoomPainter {
    private:
        int radius;

    public:
        RoomPainter(int);
        virtual ~RoomPainter(void){}

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

struct LevelGenerationFailure : public std::runtime_error {
    LevelGenerationFailure(const std::string& reason) :
        std::runtime_error( reason )
    {
    }
};

class LevelGenerator {
    protected:
        MTRand_int32& prng;
        DungeonSketch sketch;

    public:
        LevelGenerator(MTRand_int32&);

        virtual void generate(void) = 0;
        DungeonSketch& getSketch(void) { return sketch; }
};

class SimpleLevelGenerator : public LevelGenerator {
    private:
        struct PainterEntry {
            RoomPainter *painter;
            int weight;
            bool countTowardsTarget;
            bool adopted;

            PainterEntry(RoomPainter*,int,bool,bool);
        };

        std::vector<PainterEntry> entries;

        int roomTarget;
        bool ensureConnectedness;
        bool forbidTrivialLoops;
        int maxCorridors;
        bool useCorridorLengthLimit;
        int corridorLengthLimit;
        bool useRadiusLimit;
        int radiusLimit;
        bool shortestCorridorFirst;
        bool stopWhenConnected;
        int swcExtraCorridors;

        PainterEntry& selectPainter(void);
        HexTools::HexCoordinate placeRoomNear(int,int,int);

        bool pointCorridorCandidatesGenerated;
        std::vector< PointCorridor > pccs;
        int corridorCount;

        void generateRooms(void);
        bool generatePointCorridor(void);
        bool checkPCC( PointCorridor& );
        void generatePCCs(void);
        bool isConnected(void);

        void finalChecks(void);
    
    public:
        SimpleLevelGenerator(MTRand_int32&);
        ~SimpleLevelGenerator(void);

        void setRoomTarget(int);
        void adoptPainter(RoomPainter*,int,bool);

        void setShortestCorridorsFirst(bool = true);
        void setStopWhenConnected(bool = true);
        void setSWCExtraCorridors(int);

        void generate(void);
};

};

#endif
