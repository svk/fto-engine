#ifndef H_TAC_SERVER
#define H_TAC_SERVER

#include "Tac.h"

#include "mtrand.h"
#include <vector>
#include <map>
#include <set>
#include "HexTools.h"

#include "TacRules.h"

#include "Manager.h"

#include "HexFov.h"

#include "SProto.h"

#include "Turns.h"

#include <gmpxx.h>

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

class IdGenerator { // this actually DOES need high-quality seeding, for security, if we're picky
                    // (and if we're not picky what's the use of randomizing IDs at all?)
    private:
        MTRand_int32 prng;
        std::set< int > usedIds;

    public:
        IdGenerator(int seed) : prng ( seed ), usedIds () {};

        void addUsed(int);
        int generate(void);
};

class ServerUnit;
class ServerTile;

struct ServerColour {
    int r, g, b;
    ServerColour(int r, int g, int b) : r(r), g(g), b(b) {}

    Sise::SExp *toSexp(void) const;
};

class ServerColourPool { // terrible
    private:
        int index;
        std::vector<ServerColour> colours;

    public:
        ServerColourPool(void) : index(0), colours() {}

        void add(int r, int g, int b) {
            colours.push_back( ServerColour(r,g,b) );
        }

        ServerColour next(void) {
            ServerColour rv = colours[index];
            index = (index+1) % colours.size();
            return rv;
        }
};

class ServerPlayer {
    private:
        int id;
        const std::string username;
        HexTools::HexMap<const TileType*> memory;
        HexTools::HexRegion transmittedActive;
        HexTools::HexFovRegion individualFov;
        std::vector<ServerUnit*> controlledUnits;

        SProto::Server& server; // use getConnectedUser to, well, get a connected user
        ServerMap& smap;

        ServerColour playerColour;

    public:
        ServerPlayer(SProto::Server&, ServerMap&, int, const std::string&, ServerColour);

        ServerColour getColour(void) const { return playerColour; }

        void assumeAmnesia(void) { transmittedActive.clear();}

        ServerUnit* getAnyControlledUnit(void) { if(controlledUnits.size() > 0) return controlledUnits[0]; return 0; }

        const std::string& getUsername(void) const { return username; }
        int getId(void) const { return id; }

        const HexTools::HexRegion& getTotalFov(void);
        void addControlledUnit(ServerUnit* unit) { controlledUnits.push_back( unit ); }
        void removeControlledUnit(ServerUnit*);
        int getNumberOfUnits(void) const { return controlledUnits.size(); }
        
        void gatherIndividualFov(const ServerMap&);
        void updateFov(const ServerMap&);

        bool isObserving(const ServerUnit&) const;
        bool isObserving(const ServerTile&) const;
        bool isReceivingFovFrom(const ServerUnit&) const;

        void sendMemories(void);
        void sendFovDelta(void);
        void sendPlayer(const ServerPlayer&);
        void sendUnitAP(const ServerUnit&);
        void sendUnitDisappears(const ServerUnit&);
        void sendUnitDiscovered(const ServerUnit&);
        void sendUnitDiscoveredAt(const ServerUnit&, const ServerTile&);
        void sendUnitMoved(const ServerUnit&, const ServerTile&, const ServerTile&);
        void sendPlayerTurnBegins(const ServerPlayer&, double);
        void sendMeleeAttack(const ServerUnit&, const ServerUnit&, AttackResult);

        void beginTurn(void);
};

class ServerUnit {
    private:
        const int id;
        const UnitType& unitType;
        ServerPlayer *controller;

        ActivityPoints activity;

        int hp, maxHp;

        ServerTile *tile;

    public:
        ServerUnit(int,const UnitType&);

        void applyAttack(AttackResult);

        int getHP(void) const { return hp; }
        int getMaxHP(void) const { return maxHp; }

        bool isDead(void) const;

        const ActivityPoints& getAP(void) const { return activity; }
        ActivityPoints& getAP(void) { return activity; }

        const ServerTile* getTile(void) const { return tile; }
        ServerTile* getTile(void) { return tile; }
        int getId(void) const { return id; }

        void setController(ServerPlayer*);
        ServerPlayer* getController(void) { return controller; }
        const ServerPlayer* getController(void) const { return controller; }
    
        const UnitType& getUnitType(void) const { return unitType; }

        int getLayer(void) const;

        void enterTile(ServerTile*, int);
        int leaveTile(void);

        void beginTurn(void);

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

        ServerUnit *getUnit(int j) { return units[j]; }
        const ServerUnit *getUnit(int j) const { return units[j]; }

        void setUnit(ServerUnit*, int);
        int clearUnit(const ServerUnit*);
        int findUnit(const ServerUnit*) const;

        bool mayEnter(const ServerUnit*) const;
};

class ServerMap : public HexTools::HexOpacityMap {
    private:
        int mapSize;

        MTRand_int32 prng;

        IdGenerator unitIdGen;
        IdGenerator playerIdGen;

        HexTools::HexMap<ServerTile> tiles;

        std::map<int, ServerPlayer*> players;
        std::map<int, ServerUnit*> units;

        gmp_randclass gmpPrng;

        void evtUnitAppears(ServerUnit&, ServerTile&);
        void evtUnitDisappears(ServerUnit&, ServerTile&);
        void evtUnitMoved(ServerUnit&, ServerTile&, ServerTile&);
        void evtMeleeAttack(ServerUnit&, ServerUnit&, AttackResult);
        void evtUnitActivityChanged(ServerUnit&);

    public:
        ServerMap(int,TileType*,int);
        ~ServerMap(void);

        MTRand_int32& getPrng(void) { return prng; }

        void reinitialize(TileType*);

        ServerPlayer* getPlayerByUsername(const std::string&);

        int generatePlayerId(void) { return playerIdGen.generate(); }
        int generateUnitId(void) { return unitIdGen.generate(); }

        ServerPlayer* getPlayerById(int);
        ServerUnit* getUnitById(int);

        void adoptPlayer(ServerPlayer*);
        void adoptUnit(ServerUnit*);

        ServerTile& getTile(int x, int y) { return tiles.get(x,y); }
        const ServerTile& getTile(int x, int y) const { return tiles.get(x,y); }

        int getMapSize(void) const { return mapSize; }

        ServerTile* getRandomTileFor(const ServerUnit*);
        ServerTile* getTileForNear(const ServerUnit*, int, int);

        bool isOpaque(int,int) const;


        // the "cmd" family handle direct responses from the player, responses
        // which may be unreasonable. return true for success or false for failure
        // of any kind. may send information to the player (d'oh), even on failure.
        // [such as reason for failure]
        bool cmdMoveUnit(ServerPlayer*,int,int,int);
        bool cmdMeleeAttack(ServerPlayer*,int,int);

        // actions, like cmds, but originate from the server and so authority
        // does not need to be checked
        // return bool for delegation purposes (many cmds can be -- check auth -- delegate)
        bool actionMoveUnit(ServerUnit*,int,int);
        bool actionPlaceUnit(ServerUnit*,int,int);
        bool actionRemoveUnit(ServerUnit*);
        void actionPlayerTurnBegins(ServerPlayer&, double);
        bool actionMeleeAttack(ServerUnit&,ServerUnit&);
        void actionNewPlayer(ServerPlayer&);

};

void trivialLevelGenerator(ServerMap&, TileType*, TileType*, double = 0.5);

class TacTestServer : public SProto::SubServer {
    // this is a "test" server because it contains only one "game", and that game doesn't
    // really look much like Tac yet -- everyone moves at once, for one
    private:
        ServerMap myMap;

        FischerTurnManager turns;

        std::set<std::string> clients;

        ResourceManager<TileType> tileTypes;
        ResourceManager<UnitType> unitTypes;

        std::set<std::string> defeatedPlayers;
        ServerColourPool colourPool;

    public:
        TacTestServer(SProto::Server&, int, const std::string&, const std::string&, int );

        bool handle( SProto::RemoteClient*, const std::string&, Sise::SExp* );
        void tick(double dt);

        bool hasTurn(ServerPlayer*);
        void announceTurn(void);

        void delbroadcast(Sise::SExp*);

        void checkWinLossCondition(void);
        void spawnPlayerUnits(ServerPlayer*);
};

};

#endif
