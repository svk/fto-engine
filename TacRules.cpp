#include "TacRules.h"

#include <stdexcept>

#include <cassert>
#include <queue>

namespace Tac {

ActivityPoints::ActivityPoints(const UnitType& unitType, int mp, int ap, int fp) :
    speed( unitType.speed ),
    movementPoints ( mp ),
    actionPoints ( ap ),
    flexPoints ( fp ),
    movementEnergy ( 0 )
{
}

ActivityPoints::ActivityPoints(const ActivityPoints& that) :
    speed( that.speed ),
    movementPoints ( that.movementPoints ),
    actionPoints ( that.actionPoints ),
    flexPoints ( that.flexPoints ),
    movementEnergy ( that.movementEnergy )
{
}

const ActivityPoints& ActivityPoints::operator=(const ActivityPoints& that) {
    if( this != &that ) {
        speed = that.speed;
        movementPoints = that.movementPoints;
        actionPoints = that.actionPoints;
        flexPoints = that.flexPoints;
        movementEnergy = that.movementEnergy;
    }
    return *this;
}

int ActivityPoints::getImmediateMovementEnergy(void) const {
    // this is for highlighting zones, so it's somewhat informally
    // defined
    assert( movementEnergy >= 0 );
    if( movementEnergy > 0 ) {
        // what you can get by spending nothing
        return movementEnergy;
    } else if( movementPoints > 0 ) {
        // what you can get by spending one movement point
        return speed;
    }
    // if you have to use a flexibility point, you have no imm. energy
    return 0;
}

int ActivityPoints::getPotentialMovementEnergy(void) const {
    return movementEnergy + (movementPoints+flexPoints) * speed;
}

bool ActivityPoints::maySpendMovementEnergy(int cost) const {
    return getPotentialMovementEnergy() >= cost;
}

bool ActivityPoints::maySpendActionPoints(int points) const {
    return (actionPoints + flexPoints) >= points;
}

void ActivityPoints::spendMovementEnergy(int cost) {
    if( movementEnergy >= cost ) {
        movementEnergy -= cost;
    } else {
        cost -= movementEnergy;
        movementEnergy = 0;
        while( movementPoints > 0 && movementEnergy < cost ) {
            --movementPoints;
            movementEnergy += speed;
        }
        if( movementEnergy >= cost ) {
            movementEnergy -= cost;
        } else {
            while( flexPoints > 0 && movementEnergy < cost ) {
                --flexPoints;
                movementEnergy += speed;
            }
            if( movementEnergy >= cost ) {
                movementEnergy -= cost;
            } else throw std::logic_error( "could not spend movement energy" );
        }
    }
}

void ActivityPoints::spendActionPoint(int cost) {
    if( actionPoints >= cost ) {
        actionPoints -= cost;
    } else if( (actionPoints + flexPoints) >= cost ) {
        cost -= actionPoints;
        actionPoints = 0;
        flexPoints -= cost;
    } else {
        throw std::logic_error( "could not spend action points" );
    }
}

void findAllAccessible(const UnitType& unitType, const TileTypeMap& ttMap, int cx, int cy, int energy, HexTools::HexRegion& region) {
    using namespace HexTools;

    static const int dx[] = { 3, 0, -3, -3, 0, 3 },
                     dy[] = { 1, 2, 1, -1, -2, -1 };

    region.add( cx, cy );

    SparseHexMap<int> costs ( -1 );
    costs.get( cx, cy ) = 0;
    std::queue<HexCoordinate> q;
    q.push( HexCoordinate(0,0) );
    region.add( cx, cy );

    while( !q.empty() ) {
        HexCoordinate coord = q.front();
        q.pop();

        region.add( coord.first, coord.second );

        int cost = costs.get( coord.first, coord.second );

        for(int i=0;i<6;i++) {
            const int x = coord.first + dx[i], y = coord.second + dy[i];
            const TileType& tt = ttMap.getTileTypeAt( x, y );
            int traversalCost;
            if( tt.mayTraverse( unitType, traversalCost ) ) {
                if( (cost + traversalCost) > energy ) continue;
                int oldCost = costs.get(x,y);
                if( oldCost < 0 || oldCost > (cost + traversalCost) ) {
                    costs.get(x,y) = cost + traversalCost;
                }
                if( oldCost < 0 ) {
                    q.push( HexCoordinate(x,y) );
                }
            }
        }
    }

}



};
