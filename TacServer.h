#ifndef H_TAC_SERVER
#define H_TAC_SERVER

#include "Tac.h"

#include "mtrand.h"
#include <vector>
#include <set>
#include "HexTools.h"

#include "Manager.h"

#include "HexFov.h"


/* Thoughts.
   
   Server needs: - to keep track of the REAL map,
                   real unit placement, etc.
                 - to keep track of FOVs for everyone,
                   their remembered tiles (inc. remembered
                   tile type even if now invalid)
                   [ this is for recovery ]
                 - to keep a record of every change that
                   happens in the game, culminating in
                   a game record (replayable)
                 - to keep track of units, and be able
                   to generate new IDs

  Important deltas:
        put-unit <id>
        normal-move-unit <id> <fromPos> <toPos>
        melee-attack <id> <victimId> <result>
        melee-counterattack <id> <victimId> <result>
        remove-unit <id>

*/

namespace Tac {

class ServerMap;

class RememberedInformation {
    // for a single player
    // this is only used for recovery
    private:
        HexTools::HexMap<TileType*> memory;
        // 0 is valid and means not remembered

    public:
        RememberedInformation(int);
};

class IdGenerator {
    private:
        MTRand_int32 prng;
        std::set< int > usedIds;

    public:
        void addUsed(int);
        int generate(void);
};

class ServerUnit;
class ServerTile;

class ServerPlayer {
    private:
        int id;
        std::string username;
        RememberedInformation memory;
        HexTools::HexRegion transmittedActive;
        HexTools::HexFovRegion individualFov;
        std::vector<ServerUnit*> controlledUnits;

    public:
        ServerPlayer(int, const std::string&, const ServerMap&);

        void addControlledUnit(ServerUnit* unit) { controlledUnits.push_back( unit ); }
        void removeControlledUnit(ServerUnit*);
        
        void gatherIndividualFov(const ServerMap&);
};

class ServerUnit {
    private:
        int id;
        const UnitType& unitType;
        ServerPlayer *controller;

        ServerTile *tile;

    public:
        ServerUnit(int,const UnitType&);

        void setController(ServerPlayer*);
    
        const UnitType& getUnitType(void) const { return unitType; }

        int getLayer(void) const;

        void enterTile(ServerTile*, int);
        int leaveTile(void);

        void gatherFov( const ServerMap&, HexTools::HexFovRegion& ) const;
};

class ServerTile {
    private:
        TileType* tileType; // shan't be null but can change
                            // however, will be null when first constructing
                            // the map for practical reasons
        ServerUnit *units[ UNIT_LAYERS ];

        int x, y;

    public:
        ServerTile(void);
        void setXY(int x_, int y_) { x = x_; y = y_; }
        void getXY(int& x_, int& y_) const { x_ = x; y_ = y; }

        void setTileType(TileType*tt) { tileType = tt; }
        const TileType& getTileType(void) const { return *tileType; }

        void setUnit(ServerUnit*, int);
        int clearUnit(const ServerUnit*);
        int findUnit(const ServerUnit*) const;

        bool mayEnter(const ServerUnit*) const;
};

class ServerMap : public HexTools::HexOpacityMap {
    private:
        int mapSize;

        ResourceManager<ServerPlayer> players; // indexed by username
        IdGenerator unitIdGen;
        IdGenerator playerIdGen;

        HexTools::HexMap<ServerTile> tiles;

    public:
        ServerMap(int,TileType*);

        ServerTile& getTile(int x, int y) { return tiles.get(x,y); }
        const ServerTile& getTile(int x, int y) const { return tiles.get(x,y); }

        int getMapSize(void) const { return mapSize; }

        ServerTile* getRandomTileFor(const ServerUnit*);

        bool isOpaque(int,int) const;
};

void trivialLevelGenerator(ServerMap&, TileType*, TileType*, double = 0.5);

};

#endif
