#include "Tac.h"

namespace Tac {

UnitType::UnitType(const std::string& name) :
    name ( name )
{
}

TileType::TileType(const std::string& name, Type::Mobility mobility, Type::Opacity opacity, bool border, int baseCost ) :
    name ( name ),
    mobility ( mobility ),
    opacity ( opacity ),
    border ( border ),
    baseCost ( baseCost )
{
}

bool TileType::mayTraverse(const UnitType& unitType) const {
    int disregardThat;
    return mayTraverse( unitType, disregardThat );
}

bool TileType::mayTraverse(const UnitType& unitType, int& outCost) const {
    if( border ) return false;
    if( mobility == Type::WALL ) return false;
    outCost = baseCost;
    return true;
}

};
