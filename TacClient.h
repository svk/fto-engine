#ifndef H_TAC_CLIENT
#define H_TAC_CLIENT

#include "typesetter.h"

#include "HexFov.h"

#include "sftools.h"
#include "HexTools.h"
#include "hexfml.h"

#include <map>
#include <vector>
#include <string>
#include <queue>

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "anisprite.h"

#include "Tac.h"

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


struct SpriteId {
    enum Variant {
        NORMAL = 0,
        GRAYSCALE,
        NUMBER_OF_SPRITE_VARIANTS
    };

    static std::map< std::string, int > spritenoAliases;

    static void bindAlias(const std::string& name, int spriteno) {
        spritenoAliases[ name ] = spriteno;
    }

    int spriteno;
    Variant variant;

    SpriteId(const std::string& spriteAlias, Variant variant) :
        spriteno ( spritenoAliases[ spriteAlias ] ), // crashing on unbound is appropriate
        variant ( variant )
    {
    }

    SpriteId(int spriteno, Variant variant) :
        spriteno ( spriteno ),
        variant ( variant )
    {
    }
    
    SpriteId( const SpriteId& that ) :
        spriteno ( that.spriteno ),
        variant ( that.variant )
    {
    }

    const SpriteId& operator=(const SpriteId& that) {
        if( this != &that ) {
            spriteno = that.spriteno;
            variant = that.variant;
        }
        return *this;
    }

    bool operator==(const SpriteId& that) const {
        return spriteno == that.spriteno && variant == that.variant;
    }

    bool operator<(const SpriteId& that) const {
        if( spriteno < that.spriteno ) return true;
        if( spriteno == that.spriteno && variant < that.variant ) return true;
        return false;
    }
};

typedef SimpleKeyedSpritesheet<SpriteId> TacSpritesheet;

class CurveAnimation {
    public:
        virtual ~CurveAnimation(void) {}
        virtual void animate(double) = 0;
        virtual bool done(void) const = 0;
        virtual void get(double&,double&) const = 0;
        virtual bool affectsBasePosition(void) { return true; }
};

class LineCurveAnimation : public CurveAnimation {
    private:
        const double duration;
        double x0, y0, x1, y1;
        double phase;
        bool affectsBase;

    public:
        LineCurveAnimation(double,double,double,double,double, bool);

        void animate(double);
        bool done(void) const;
        void get(double&,double&) const;

        virtual bool affectsBasePosition(void) { return affectsBase; }
};

// note: a "unit type / tile type" includes all sorts of
// variants (that give bonuses/penalties or aesthetic
// variations) that are not completely individual to a
// unit (and it's not clear that I'll need the latter).
// so, if Italian troops have +1 movement, and English
// troops +1 damage, then an "Italian crossbowman" is
// a different unit type from an "English crossbowman".
struct ClientUnitType : public UnitType {
    sf::Sprite spriteNormal;

    ClientUnitType(TacSpritesheet&, const std::string&, const std::string&);
};

struct ClientTileType : public TileType {
    sf::Sprite spriteNormal, spriteGrayscale;

    ClientTileType(TacSpritesheet&,
                   const std::string&, // sprite alias
                   const std::string&, // name
                   Type::Mobility,
                   Type::Opacity,
                   bool,
                   int);
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

        const ClientUnitType& getUnitType(void) const { return unitType; }

        void getCenterOffset(int&, int&, bool) const;

        int getId(void) const;

        void setLiving(bool);
        void setPosition(int,int);
        void setNoPosition(void);

        void enterTile(ClientTile&, int);
        int leaveTile(ClientTile&);
        void move(int,int);

        bool getPosition(int&, int&) const;
        int getLayer(void) const { return layer; }

        void animate(double);
        bool done(void) const;
        void completedAnimation(void);

        void startMovementAnimation(int,int);
        void startMeleeAnimation(int,int);
};

class ClientUnitManager {
    private:
        std::map<int,ClientUnit*> units;

    public:
        ~ClientUnitManager(void);

        void adopt(ClientUnit*);

        ClientUnit* operator[](int) const; // may return 0!
};

class CMLevelBlitterGL;

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
        void setTileType(ClientTileType*);

        void setHighlight( Highlight );
        Highlight getHighlight(void) const { return highlight; }

        int clearUnitById(int);
        void setUnit(int, int);

        int getUnitIdAt(int j) const { return unitId[j]; }

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

class CMUnitBlitterGL : public HexBlitter {
    private:
        ClientMap& cmap;
        TacSpritesheet& unitsheet;
        int layer;

    public:
        CMUnitBlitterGL( ClientMap& cmap,
                         TacSpritesheet& unitsheet,
                         int layer ) :
            cmap ( cmap ),
            unitsheet ( unitsheet ),
            layer ( layer )
        {
        }

        void drawHex(int, int, sf::RenderWindow&);
};

class CMLevelBlitterGL : public HexBlitter {
    private:
        ClientMap& cmap;
        TacSpritesheet& tilesheet;
        
        sf::Sprite spriteFogZone,
                   spriteMoveZone,
                   spriteAttackZone,
                   spriteThinGrid;

    public:
        CMLevelBlitterGL( ClientMap& cmap,
                          TacSpritesheet& tilesheet ) :
            cmap ( cmap ),
            tilesheet ( tilesheet ),
            spriteFogZone ( tilesheet.makeSprite( SpriteId("zone-fog", SpriteId::NORMAL ) ) ),
            spriteMoveZone ( tilesheet.makeSprite( SpriteId("zone-move", SpriteId::NORMAL ) ) ),
            spriteAttackZone ( tilesheet.makeSprite( SpriteId("zone-attack", SpriteId::NORMAL ) ) ),
            spriteThinGrid ( tilesheet.makeSprite( SpriteId("grid-thin", SpriteId::NORMAL ) ) )
        {
        }

        void putSprite( const sf::Sprite& sprite );
        void drawHex(int, int, sf::RenderWindow&);
};


class ClientMap : public HexOpacityMap {
    private:
        FreetypeFace* risingTextFont;

        int radius;
        HexTools::HexMap<ClientTile> tiles;
        ClientUnitManager units;

        ClientActionQueue caq;

        ClientUnit *animatedUnit;

        CMLevelBlitterGL levelBlitter;
        CMUnitBlitterGL groundUnitBlitter;

        HexRegion moveHighlightZone, attackHighlightZone;

        HexRegion activeRegion;

        ScreenGrid& grid;

        typedef FiniteLifetimeObjectList<RisingTextAnimation> RTAManager;
        RTAManager animRisingText;

        std::vector< sf::Sound* > soundsPlaying;

        bool shouldBlock(void) const;
        bool isInBlockingAnimation(void) const;

    public:
        ClientMap(int, TacSpritesheet&, ScreenGrid&, FreetypeFace*);

        ScreenGrid& getGrid(void) const { return grid; }

        void setAnimatedUnit( ClientUnit* unit ) { animatedUnit = unit; }

        bool isOpaque(int,int) const;

        CMLevelBlitterGL& getLevelBlitter(void) { return levelBlitter; }
        CMUnitBlitterGL& getUnitBlitter(int);

        void updateActive(const HexTools::HexRegion&);

        void adoptUnit(ClientUnit*);

        void playSound( sf::SoundBuffer* );

        // low-level movement!
        void placeUnitAt(int,int,int,int);
        void moveUnit(int,int,int);

        void removeUnit(int);

        void animate(double dt);

        void queueAction(ClientAction*); // adopts
        void processActions(void);

        ClientTile& getTile(int x, int y) { return tiles.get(x,y); }
        ClientUnit* getUnitById(int);
        const ClientUnit* getUnitById(int) const;

        void setTileType(int, int, ClientTileType*);

        void clearHighlights(void);
        void addMoveHighlight(int, int);
        void addAttackHighlight(int, int);

        void addRisingText(int,int,const std::string&, const sf::Color&, int);

        bool unitMayMove(int,int,int) const;
        bool unitMayMoveTo(int,int,int) const;

        bool getUnitScreenPositionById( int, double&, double& ) const;
        bool getUnitBaseScreenPositionById( int, double&, double& ) const;
        void drawEffects(sf::RenderWindow&, double, double);
};


};

#endif
