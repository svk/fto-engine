#ifndef H_TAC
#define H_TAC

#include <string>

#define INVALID_ID 0
#define UNIT_LAYERS 1

#include "Manager.h"

namespace Tac {

// this contains what both the server and the client need to know about
// unit/tile types. the server is not graphical and as such is not
// interested in actually loading sprites.
// there is no hidden information on this level; no information that a
// client SHOULD NOT know.
// [ this is because of the design philosophy of having odds in the
//   clear; if the client allows various projections/calculations,
//   that's just good UI. ]
// generally though, all unit types should be sent to the client if sent
// over the wire: the members of the unit set/tile set should not carry
// information about the unexplored map.
// exception: PvM maps, where it's okay to keep the unit types secret
//  until you reach the map and only semi-secret once you have. still,
//  whole sets should be sent at once ( "dragons", "kobolds", etc. )
//  shouldn't be possible to tell -- "aha, the kobold king is on this map"
//  before spotting it

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

struct UnitType {
    std::string symbol;
    std::string name;

    int nativeLayer;

    UnitType(const std::string&, const std::string&);
    virtual ~UnitType(void) {}
};

struct TileType {
    std::string symbol; // this can double as the sprite alias
    std::string name; // the name is NOT the sprite alias -- want to have possibility of many sprites per name

    Type::Mobility mobility;
    Type::Opacity opacity;
    bool border;
    int baseCost;

    bool mayTraverse(const UnitType&) const;
    bool mayTraverse(const UnitType&, int&) const;

    TileType(const std::string&,
             const std::string&, // name
             Type::Mobility,
             Type::Opacity,
             bool,
             int);
    virtual ~TileType(void) {}
};

};

#endif
