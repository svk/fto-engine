#ifndef H_TAC_SERVER
#define H_TAC_SERVER

#include "Tac.h"

#include "mtrand.h"
#include <vector>
#include <map>
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
        void updateFov(const ServerMap&);

        bool isObserving(const ServerTile&) const;

        void sendUnitDisappears(const ServerUnit&);
        void sendUnitDiscovered(const ServerUnit&);
        void sendUnitMoved(const ServerUnit&, const ServerTile&, const ServerTile&);
};

class ServerUnit {
    private:
        const int id;
        const UnitType& unitType;
        ServerPlayer *controller;

        ServerTile *tile;

    public:
        ServerUnit(int,const UnitType&);

        ServerTile* getTile(void) { return tile; }
        int getId(void) const { return id; }

        void setController(ServerPlayer*);
        ServerPlayer* getController(void) { return controller; }
    
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

        IdGenerator unitIdGen;
        IdGenerator playerIdGen;

        HexTools::HexMap<ServerTile> tiles;

        std::map<int, ServerPlayer*> players;
        std::map<int, ServerUnit*> units;

        void evtUnitAppears(ServerUnit&, ServerTile&);
        void evtUnitDisappears(ServerUnit&, ServerTile&);
        void evtUnitMoved(ServerUnit&, ServerTile&, ServerTile&);

    public:
        ServerMap(int,TileType*);
        ~ServerMap(void);

        int generatePlayerId(void) { return playerIdGen.generate(); }
        int generateUnitId(void) { return unitIdGen.generate(); }

        ServerUnit* getUnitById(int);

        void adoptUnit(ServerUnit*);

        ServerTile& getTile(int x, int y) { return tiles.get(x,y); }
        const ServerTile& getTile(int x, int y) const { return tiles.get(x,y); }

        int getMapSize(void) const { return mapSize; }

        ServerTile* getRandomTileFor(const ServerUnit*);

        bool isOpaque(int,int) const;


        // the "cmd" family handle direct responses from the player, responses
        // which may be unreasonable. return true for success or false for failure
        // of any kind. may send information to the player (d'oh), even on failure.
        // [such as reason for failure]
        bool cmdMoveUnit(ServerPlayer*,int,int,int);

        // actions, like cmds, but originate from the server and so authority
        // does not need to be checked
        // return bool for delegation purposes (many cmds can be -- check auth -- delegate)
        bool actionMoveUnit(ServerUnit*,int,int);
        bool actionPlaceUnit(ServerUnit*,int,int);
        bool actionRemoveUnit(ServerUnit*);

};

void trivialLevelGenerator(ServerMap&, TileType*, TileType*, double = 0.5);

};

#endif
