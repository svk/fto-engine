#include "TacClient.h"

#include "TacRules.h"

namespace Tac {

struct PlaySoundCAction : public ClientAction {
    ClientMap& cmap;
    sf::SoundBuffer& buffer;

    PlaySoundCAction(ClientMap& cmap, sf::SoundBuffer& buffer) :
        cmap ( cmap ), buffer ( buffer ) {}
    ClientAction* duplicate(void) const { return new PlaySoundCAction( cmap, buffer ); }
    void operator()(void) const;
    bool isCosmetic(void) const { return true; }

};

struct ApplyAttackResultSubstantialCAction : public ClientAction {
    ClientMap& cmap;
    int attackerId, defenderId;
    AttackResult result;

    ApplyAttackResultSubstantialCAction(ClientMap& cmap, int attackerId, int defenderId, AttackResult result) :
        cmap( cmap ), attackerId( attackerId ), defenderId( defenderId ), result ( result )
    {
    }

    ClientAction* duplicate(void) const { return new ApplyAttackResultSubstantialCAction( cmap, attackerId, defenderId, result ); }

    void operator()(void) const;

    bool isCosmetic(void) const { return false; }
};

struct ApplyAttackResultCosmeticCAction : public ClientAction {
    ClientMap& cmap;
    int attackerId, defenderId;
    AttackResult result;

    ApplyAttackResultCosmeticCAction(ClientMap& cmap, int attackerId, int defenderId, AttackResult result) :
        cmap( cmap ), attackerId( attackerId ), defenderId( defenderId ), result ( result )
    {
    }

    ClientAction* duplicate(void) const { return new ApplyAttackResultCosmeticCAction( cmap, attackerId, defenderId, result ); }

    void operator()(void) const;

    bool isCosmetic(void) const { return true; }
};


struct NormalMovementCAction : public ClientAction {
    ClientMap& cmap;
    int unitId;
    int dx, dy;

    NormalMovementCAction(ClientMap& cmap, int unitId, int dx, int dy) :
        cmap( cmap ), unitId( unitId ), dx ( dx ), dy ( dy )
    {
    }

    ClientAction* duplicate(void) const { return new NormalMovementCAction( cmap, unitId, dx, dy ); }

    void operator()(void) const;

    bool isCosmetic(void) const { return false; }
};

struct DarkenCAction : public ClientAction {
    ClientMap& cmap;
    std::vector<std::pair<int,int> > darkenTiles;

    void add(int x, int y) {
        darkenTiles.push_back( std::pair<int,int>( x, y ) );
    }

    DarkenCAction(ClientMap& cmap) :
        cmap( cmap ), darkenTiles ()
    {
    }

    DarkenCAction(const DarkenCAction& that ) :
        cmap( that.cmap ), darkenTiles ( darkenTiles )
    {
    }

    ClientAction* duplicate(void) const { return new DarkenCAction( *this ); }

    void operator()(void) const;

    bool isCosmetic(void) const { return false; }
};

struct BrightenCAction : public ClientAction {
    ClientMap& cmap;

    struct BrightenTile {
        int x, y;
        const ClientTileType *tt;
        BrightenTile(int x, int y, const ClientTileType* tt) :
            x ( x ) , y ( y ), tt ( tt )
        {}
    };

    std::list<BrightenTile> brightenTiles;

    void add(int x, int y) {
        brightenTiles.push_back( BrightenTile(x,y,0) );
    }

    void add(int x, int y, const ClientTileType*tt) {
        brightenTiles.push_back( BrightenTile(x,y,tt) );
    }

    BrightenCAction(ClientMap& cmap) :
        cmap( cmap ), brightenTiles ()
    {
    }

    BrightenCAction(const BrightenCAction& that ) :
        cmap( that.cmap ), brightenTiles ( brightenTiles )
    {
    }

    ClientAction* duplicate(void) const { return new BrightenCAction( *this ); }

    void operator()(void) const;

    bool isCosmetic(void) const { return false; }
};


struct SetActiveRegionCAction : public ClientAction {
    // as noted below, this needn't actually be so large
    ClientMap& cmap;

    HexTools::HexRegion region;

    void add(int x, int y) {
        region.add( x, y );
    }

    SetActiveRegionCAction(ClientMap& cmap ) :
        cmap( cmap ), region ( )
    {
    }

    SetActiveRegionCAction(const SetActiveRegionCAction& that ) :
        cmap( that.cmap ), region ( that.region )
    {
    }

    ClientAction* duplicate(void) const { return new SetActiveRegionCAction( *this ); }

    void operator()(void) const;

    bool isCosmetic(void) const { return true; }
};

struct BeginPlayerTurnCAction : public ClientAction {
    ClientMap& cmap;
    int playerId;

    BeginPlayerTurnCAction(ClientMap& cmap, int playerId) :
        cmap ( cmap ),
        playerId ( playerId )
    {
    }

    ClientAction* duplicate(void) const { return new BeginPlayerTurnCAction( cmap, playerId ); }

    void operator()(void) const;

    bool isCosmetic(void) const { return false; }
};

struct RemoveUnitCAction : public ClientAction {
    ClientMap& cmap;
    int unitId;

    RemoveUnitCAction(ClientMap& cmap, int unitId) :
        cmap ( cmap ),
        unitId ( unitId )
    {
    }

    ClientAction* duplicate(void) const { return new RemoveUnitCAction( cmap, unitId ); }

    void operator()(void) const;

    bool isCosmetic(void) const { return false; }
};


struct RisingTextCAction : public ClientAction {
    ClientMap& cmap;
    int hexX, hexY;
    std::string text;
    int r, g, b, a;

    RisingTextCAction(ClientMap& cmap, int hexX, int hexY, const std::string& text, int r, int g, int b, int a ) :
        cmap ( cmap ),
        hexX ( hexX ),
        hexY ( hexY ),
        text ( text ),
        r ( r ),
        g ( g ),
        b ( b ),
        a ( a )
    {
    }

    ClientAction* duplicate(void) const { return new RisingTextCAction( cmap, hexX, hexY, text, r, g, b, a ); }

    void operator()(void) const;

    bool isCosmetic(void) const { return true; }
};

struct RevealTerrainCAction : public ClientAction {
    // new information about terrain
    // may come separately from active set because we need to
    //  send the active set a lot and want it to be as small
    //  as possible
    // [ strictly, if we run into trouble with inefficiency,
    //   couldn't we calculate the active set client-side,
    //   since we'll know the positions of all our active
    //   units? hmm. ]

    ClientMap& cmap;

    typedef std::map< std::pair<int,int>, const ClientTileType* > RevelationsType;
    RevelationsType revelations;

    void add(int x, int y, const ClientTileType* ctt) {
        revelations[std::pair<int,int>(x,y)] = ctt;
    }

    RevealTerrainCAction(ClientMap& cmap ) :
        cmap( cmap ), revelations ( )
    {
    }

    RevealTerrainCAction(const RevealTerrainCAction& that ) :
        cmap( that.cmap ), revelations ( that.revelations )
    {
    }

    ClientAction* duplicate(void) const { return new RevealTerrainCAction( *this ); }

    void operator()(void) const;

    bool isCosmetic(void) const { return false; }
};

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

struct BumpAnimationCAction : public ClientAction {
    ClientMap& cmap;
    int unitId;
    int dx, dy;

    BumpAnimationCAction(ClientMap& cmap, int unitId, int dx, int dy) :
        cmap( cmap ), unitId( unitId ), dx ( dx ), dy ( dy )
    {
    }

    ClientAction* duplicate(void) const { return new BumpAnimationCAction( cmap, unitId, dx, dy ); }

    void operator()(void) const;

    bool isCosmetic(void) const { return true; }
};

struct UnitDiscoverCAction : public ClientAction {
    ClientMap& cmap;
    int unitId;
    ClientUnitType& unitType;
    int team, owner;
    int x, y, layer;
    ActivityPoints ap;
    int hp, maxHp;

    UnitDiscoverCAction(ClientMap& cmap, int unitId, ClientUnitType& unitType, int team, int owner, int x, int y, int layer, ActivityPoints ap, int hp, int maxHp) :
        cmap( cmap ), unitId( unitId ), unitType ( unitType ), team (team), owner( owner ), x ( x ), y ( y ), layer ( layer ), ap ( ap ), hp ( hp ), maxHp ( maxHp )
    {
    }

    ClientAction* duplicate(void) const { return new UnitDiscoverCAction( cmap, unitId, unitType, team, owner, x, y, layer, ap, hp, maxHp ); }

    void operator()(void) const;

    bool isCosmetic(void) const { return false; }
};

};
