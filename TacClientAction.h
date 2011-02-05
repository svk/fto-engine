#include "TacClient.h"

namespace Tac {

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

    typedef std::map< std::pair<int,int>, ClientTileType* > RevelationsType;
    RevelationsType revelations;

    void add(int x, int y, ClientTileType* ctt) {
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

    bool isCosmetic(void) const { return true; }
};

struct BumpAnimationCAction : public ClientAction {
    // this animation is an animation of movement, but it is also used
    // as an attack animation -- as such, obv. it has no actual movement
    // semantics. (in fact, the plan is that actions should be either
    // purely cosmetic or purely functional.)

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


};
