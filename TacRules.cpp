#include "TacRules.h"

#include <stdexcept>

#include <iostream>

#include <cassert>
#include <queue>

namespace Tac {

const static double OBRParameterToHitP = 0.75;
const static double OBRParameterToHitC = 0.06;

const static double OBRParameterDamageP = 0.5;
const static double OBRParameterDamageC = 0.02;

AttackRoller::AttackRoller(void) :
    OpposedBooleanRoller( OBRParameterToHitP, OBRParameterToHitC )
{
}

DamageSuccessRoller::DamageSuccessRoller(void) :
    OpposedBooleanRoller( OBRParameterDamageP, OBRParameterDamageC )
{
}


Outcomes<AttackResult> BernoulliDamageDie::transform(AttackResult x) {
    Outcomes<AttackResult> rv;
    if( x.status == AttackResult::MISS ) {
        rv.add( 1, x );
    } else {
        rv.add( mpq_class(1) - successP, x );
        x.damage += firepower;
        rv.add( successP, x );
    }
    return rv;
}

mpq_class OpposedBooleanRoller::chance(int x) {
    const int resolution = 100000;
    double rv = p;

    if( x < 0 ) {
        rv /= pow( 2, c * abs(x) );
    } else if( x > 0 ) {
        rv = 1 + (p-1) / pow( 2, (p/(1-p)) * c * x );
    }

    int irv = (int)(0.5+(rv * resolution));
    if( irv >= resolution ) {
        return mpq_class( resolution-1, resolution );
    }
    if( irv <= 0 ) {
        return mpq_class( 1, resolution );
    }
    return mpq_class( irv, resolution );
}

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

mpq_class ActivityPoints::getImmediateMovementEnergy(void) const {
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

mpq_class ActivityPoints::getPotentialMovementEnergy(void) const {
    return movementEnergy + (movementPoints+flexPoints) * speed;
}

bool ActivityPoints::maySpendMovementEnergy(mpq_class cost) const {
    using namespace std;
    return getPotentialMovementEnergy() >= cost;
}

bool ActivityPoints::maySpendActionPoints(int points) const {
    return (actionPoints + flexPoints) >= points;
}

void ActivityPoints::spendMovementEnergy(mpq_class cost) {
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

void findAllAccessible(const UnitType& unitType, const TileTypeMap& ttMap, int cx, int cy, mpq_class energy, HexTools::HexReceiver& region) {
    using namespace HexTools;

    static const int dx[] = { 3, 0, -3, -3, 0, 3 },
                     dy[] = { 1, 2, 1, -1, -2, -1 };

    region.add( cx, cy );

        using namespace std;

    SparseHexMap<mpq_class> costs ( -1 );

    costs.set( cx, cy, 0 );
    std::queue<HexCoordinate> q;
    q.push( HexCoordinate(cx,cy) );
    region.add( cx, cy );

    while( !q.empty() ) {
        HexCoordinate coord = q.front();
        q.pop();

        region.add( coord.first, coord.second );

        mpq_class cost = costs.get( coord.first, coord.second );

        for(int i=0;i<6;i++) {
            const int x = coord.first + dx[i], y = coord.second + dy[i];
            const TileType* tt = ttMap.getTileTypeAt( x, y );
            mpq_class traversalCost;
            if( tt && tt->mayTraverse( unitType, traversalCost ) ) {
                if( (cost + traversalCost) > energy ) continue;
                mpq_class oldCost = costs.get(x,y);
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

Sise::SExp* AttackResult::toSexp(void) const {
    using namespace Sise;
    if( status == MISS ) {
        return List()( new Symbol( "miss" ) ).make();
    } else if( status == HIT ) {
        return List()( new Symbol( "hit" ) )
                     ( new Int( damage ) )
               .make();
    }
    throw std::logic_error( "status neither hit nor miss" );
}

AttackResult AttackResult::fromSexp(Sise::SExp* sexp) {
    using namespace Sise;
    Cons *args = asProperCons( sexp );
    std::string type = *asSymbol( args->nthcar(0) );
    AttackResult rv;
    if( type == "miss" ) {
        rv.status = MISS;
    } else if( type == "hit" ) {
        rv.status = HIT;
        rv.damage = *asInt( args->nthcar(1) );
    } else throw std::logic_error( "status in sexp neither hit nor miss" );
    return rv;
}

Sise::SExp* ActivityPoints::toSexp(void) const {
    using namespace Sise;
    return List()( new BigRational( speed ) )
                 ( new Int( movementPoints ) )
                 ( new Int( actionPoints ) )
                 ( new Int( flexPoints ) )
                 ( new BigRational( movementEnergy ) )
           .make();
}

ActivityPoints::ActivityPoints(Sise::SExp* sexp) {
    using namespace Sise;
    Cons *args = asProperCons( sexp );
    speed = *asInt( args->nthcar(0) );
    movementPoints = *asInt( args->nthcar(1) );
    actionPoints = *asInt( args->nthcar(2) );
    flexPoints = *asInt( args->nthcar(3) );
    movementEnergy = asMPQ( args->nthcar(4) );
}

bool AttackResult::operator==(const AttackResult& that) const {
    if( status != that.status ) return false;
    if( status == MISS && that.status == MISS ) return true;
    if( status == HIT ) {
        return damage == that.damage;
    }
    throw std::logic_error( "invalid state" );
}

ActivityPoints ActivityPoints::fromSexp(Sise::SExp* sexp) {
    using namespace Sise;
    Cons *args = asProperCons( sexp );
    ActivityPoints rv;
    rv.speed = asMPQ( args->nthcar(0) );
    rv.movementPoints = *asInt( args->nthcar(1) );
    rv.actionPoints = *asInt( args->nthcar(2) );
    rv.flexPoints = *asInt( args->nthcar(3) );
    rv.movementEnergy = asMPQ( args->nthcar(4) );
    return rv;
}

Outcomes<AttackResult> makeAttack(int att, int def) {
    const mpq_class successP = AttackRoller().chance(att,def);
    Outcomes<AttackResult> rv;
    AttackResult hit, miss;
    hit.status = AttackResult::HIT;
    hit.damage = 0;
    miss.status = AttackResult::MISS;
    rv.add( successP, hit );
    rv.add( mpq_class(1) - successP, miss );
    return rv;
}

Outcomes<AttackResult> DamageDealer::transform(AttackResult x) {
    Outcomes<AttackResult> rv;
    rv.add( 1, x );
    if( x.status == AttackResult::HIT ) {
        for(int i=0;i<diceno;i++) {
            rv = BernoulliDamageDie(successP, firepower)( rv );
        }
    }
    return rv;
}

int getDamageOfAttack(AttackResult res) {
    if( res.status == AttackResult::MISS ) return 0;
    return res.damage;
}

Outcomes<AttackResult> makeAttackBetween(const AttackCapability& att, const DefenseCapability& def) {
    Outcomes<AttackResult> rv = makeAttack( att.attack, def.defense );
    rv = DamageDealer( att.attack, def.defense, att.shots, att.firepower)( rv );
    rv = DamageReducer( def.reduction )( rv );
    rv = DamageResister( def.resistance )( rv );
    return rv;
}

void ActivityPoints::forbidMovement(void) {
    movementEnergy = 0;
    movementPoints = 0;
    actionPoints += flexPoints;
    flexPoints = 0;
}


};
