#ifndef H_TAC_CLIENT
#define H_TAC_CLIENT

#define INVALID_ID 0
#define UNIT_LAYERS 1

#include "sftools.h"
#include "HexTools.h"

#include <map>
#include <vector>
#include <string>
#include <queue>

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>

namespace Tac {

// for now we have universal mobility; this
// will change -- most obvious exception
// flyers, water.
// client must know the unit type x tile type
// -> energy cost function, and the energy
// capacity for each unit. This is because
// simple pathfinding should be integrated
// into the UI and should be done without
// delay.


// impassable but clear: water or glass walls
// passable but not clear: ? (closed doors, in a sense)

// door is probably its own mobility/opacity type;
// passability/opacity depends on state
// opening a door is an ACTION (like an attack)

namespace Type {
    enum Mobility {
        FLOOR,
        WALL
    };

    enum Opacity {
        CLEAR,
        BLOCK
    };
};

class CurveAnimation {
    public:
        virtual ~CurveAnimation(void) {}
        virtual void animate(double) = 0;
        virtual bool done(void) const = 0;
        virtual void get(double&,double&) const = 0;
};

class LineCurveAnimation : public CurveAnimation {
    private:
        const double duration;
        double x0, y0, x1, y1;
        double phase;

    public:
        LineCurveAnimation(double,double,double,double,double);

        void animate(double);
        bool done(void) const;
        void get(double&,double&) const;
};

struct ClientUnitType {
    int spriteno;
};

struct ClientTileType {
    int spriteno;
    Type::Mobility mobility;
    Type::Opacity opacity;
    bool border;
    int baseCost;

    bool mayTraverse(ClientUnitType&,int&) const;
};

class ClientTile;

class ClientUnit {
    private:
        int id; // randomized so client can't tell how many units there are etc.
                // this can be FAKED in the case of e.g. hallucination, disguise
        ClientUnitType& unitType;
        int team;
        int owner;
        enum State {
            LIVING,
            DEAD
        } state; // this might not be accurate -- illusions etc

        int x, y, layer;
        bool hasPosition;

        CurveAnimation *curveAnim;

    public:
        ClientUnit(int, ClientUnitType&, int, int);

        void getCenterOffset(double&, double&) const;

        int getId(void) const;

        void setLiving(bool);
        void setPosition(int,int);
        void setNoPosition(void);

        void enterTile(ClientTile&, int);
        int leaveTile(ClientTile&);
        void move(int,int);

        bool getPosition(int&, int&);

        void animate(double);
        bool done(void) const;
        void completedAnimation(void);

        void startMovementAnimation(int,int);
};

class ClientUnitManager {
    private:
        std::map<int,ClientUnit*> units;

    public:
        ~ClientUnitManager(void);

        void adopt(ClientUnit*);

        ClientUnit* operator[](int) const; // may return 0!
};

class ClientTile {
    public:
        enum Highlight {
            NONE = 0,
            MOVE_ZONE,
            ATTACK_ZONE
        };

    private:
        ClientTileType *tileType; // 0 for unknown
        Highlight highlight;
        bool inactive; // show fog of war and don't trust completely

        int unitId[ UNIT_LAYERS ];

        int x, y;

        void clearUnits(void);

    public:
        ClientTile(void);

        ClientTileType *getTileType(void) const { return tileType; }

        void setHighlight( Highlight );

        int clearUnitById(int);
        void setUnit(int, int);

        void setActive(void);
        void setInactive(void);

        void setXY(int x_, int y_) { x = x_; y = y_; }
        int getX() const { return x; }
        int getY() const { return y; }

        bool getActive(void) const { return !inactive; }
};

// client actions take time to execute
// (because they need to be presented visually)
// so we need to represent them and queue them up
// it is ALSO desirable that we add some rewind/f-f
// functionality -- required at least to be able
// to replay the last turn
// sane way: save a snapshot each turn
// full client action queue should be enough to
// reconstruct the entire game, but not all clients
// get all actions. (when viewing a replay special
// care must be taken re: _illusory_ units -- both
// the illusion and the reality should be viewable,
// depending on the perspective chosen)
// the ones units sent from the server may be larger
// than this, especially since we split actions into
// "cosmetic" and "non-cosmetic" actions -- so
// (move 1337 3 1) is translated to MovementAnimation,
// MoveUnit, (StopAnimation).

// actions should be _POD objects.
// use coordinates and IDs, never references or pointers

class ClientAction {
    public:
        virtual ~ClientAction(void) {}

        virtual ClientAction* duplicate(void) const = 0;
        virtual void operator()(void) const = 0;
        virtual bool isCosmetic(void) const = 0;
};

class ClientActionQueue {
    private:
        std::queue< ClientAction* > actions;

    public:
        ~ClientActionQueue(void);

        bool empty(void) const;

        void adopt(ClientAction*);
        ClientAction* pop(void);
};

class ClientMap;

struct MovementAnimationCAction : public ClientAction {
    ClientMap& cmap;
    int unitId;
    int dx, dy;

    MovementAnimationCAction(ClientMap& cmap, int unitId, int dx, int dy) :
        cmap( cmap ), unitId( unitId ), dx ( dx ), dy ( dy )
    {
    }

    ClientAction* duplicate(void) const { return new MovementAnimationCAction( cmap, unitId, dx, dy ); }

    void operator()(void) const;

    bool isCosmetic(void) const { return true; }
};

class ClientMap;

class CMLevelBlitterGL {
    private:
        ClientMap& cmap;
        Spritesheet& tilesheet;

    public:
        CMLevelBlitterGL( ClientMap& cmap,
                          Spritesheet& tilesheet ) :
            cmap ( cmap ),
            tilesheet ( tilesheet )
        {
        }

        void putSprite( const sf::Sprite& sprite );
        void drawHex(int, int, sf::RenderWindow&);

};


class ClientMap {
    private:
        int radius;
        HexTools::HexMap<ClientTile> tiles;
        ClientUnitManager units;

        ClientActionQueue caq;

        ClientUnit *animatedUnit;

        bool shouldBlock(void) const;
        bool isInBlockingAnimation(void) const;

        friend class CMLevelBlitterGL;

    public:
        ClientMap(int);

        void updateActive(const HexTools::HexRegion&);

        void adoptUnit(ClientUnit*);

        // low-level movement!
        void placeUnitAt(int,int,int,int);
        void moveUnit(int,int,int);

        void animate(double dt);

        void queueAction(const ClientAction&);
        void processActions(void);

        ClientUnit* getUnitById(int);
};


};

#endif
