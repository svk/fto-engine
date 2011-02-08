#include "TacRules.h"

#include <stdexcept>

#include <iostream>

#include <cassert>
#include <queue>

namespace Tac {

ActivityPoints::ActivityPoints(void) :
    speed(0),
    movementPoints(0),
    actionPoints(0),
    flexPoints(0),
    movementEnergy(0)
{
}

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
            } else {
                using namespace std;
                cerr << "warning: overspending movement energy" << endl;
            }
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
        using namespace std;
        cerr << "warning: overspending action points" << endl;
    }
}

void findAllAccessible(const UnitType& unitType, const TileTypeMap& ttMap, int cx, int cy, int energy, HexTools::HexReceiver& region) {
    using namespace HexTools;

    static const int dx[] = { 3, 0, -3, -3, 0, 3 },
                     dy[] = { 1, 2, 1, -1, -2, -1 };

    region.add( cx, cy );

        using namespace std;

    SparseHexMap<int> costs ( -1 );

    costs.set( cx, cy, 0 );
    std::queue<HexCoordinate> q;
    q.push( HexCoordinate(cx,cy) );
    region.add( cx, cy );

    while( !q.empty() ) {
        HexCoordinate coord = q.front();
        q.pop();

        region.add( coord.first, coord.second );

        int cost = costs.get( coord.first, coord.second );

        for(int i=0;i<6;i++) {
            const int x = coord.first + dx[i], y = coord.second + dy[i];
            const TileType* tt = ttMap.getTileTypeAt( x, y );
            int traversalCost;
            if( tt && tt->mayTraverse( unitType, traversalCost ) ) {
                if( (cost + traversalCost) > energy ) continue;
                int oldCost = costs.get(x,y);
                oldCost = costs.get(x,y);
                if( oldCost < 0 || oldCost > (cost + traversalCost) ) {
                    costs.set(x,y, cost + traversalCost );
                }
                if( oldCost < 0 ) {
                    q.push( HexCoordinate(x,y) );
                }
            }
        }
    }

}



};
