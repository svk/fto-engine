#include "Tac.h"

#include "Sise.h"
#include "SProto.h"

namespace Tac {

UnitType::UnitType(const std::string& symbol, const std::string& name, int speed, int maxHp, AttackCapability *meleeAttack, DefenseCapability defense ) :
    symbol ( symbol ),
    name ( name ),
    maxHp ( maxHp ),
    speed ( speed ),
    nativeLayer ( 0 ), // xx
    meleeAttack ( meleeAttack ),
    defense ( defense )
{
}

TileType::TileType(const std::string& symbol, const std::string& name, Type::Mobility mobility, Type::Opacity opacity, bool border, int baseCost ) :
    symbol ( symbol ),
    name ( name ),
    mobility ( mobility ),
    opacity ( opacity ),
    border ( border ),
    baseCost ( baseCost )
{
}


UnitType::UnitType(Sise::SExp* sexp) {
    using namespace Sise;
    Cons *args = asProperCons( sexp );
    Cons *alist = asProperCons( args->getcdr() );

    symbol = *asSymbol( args->getcar() );
    name = *asString( alist->alistGet( "name" ) );
    maxHp = *asInt( alist->alistGet( "max-hp" ) );
    speed = asMPQ( alist->alistGet( "speed" ) );
    nativeLayer = *asInt( alist->alistGet( "native-layer" ) );

    SExp *t = alist->alistGet( "melee-attack" );
    if( t ) {
        meleeAttack = new AttackCapability( t );
    } else {
        meleeAttack = 0;
    }

    defense = DefenseCapability( alist->alistGet( "defense" ) );
}

Sise::SExp *TileType::toSexp(void) const {
    using namespace Sise;
    return List()( new Symbol( symbol ) )
                 ( new Cons( new Symbol( "name" ),
                             new String( name ) ) )
                 ( new Cons( new Symbol( "mobility" ),
                             new Symbol( ( mobility == Type::FLOOR ) ? "floor" :
                                         ( mobility == Type::WALL ) ? "wall" :
                                         "invalid" ) ) )
                 ( new Cons( new Symbol( "opacity" ),
                             new Symbol( ( opacity == Type::CLEAR ) ? "clear" :
                                         ( opacity == Type::BLOCK ) ? "block" :
                                         "invalid" ) ) )
                 ( new Cons( new Symbol( "border?" ),
                             new Symbol( ( border ) ? "yes" : "no" ) ) )
                 ( new Cons( new Symbol( "base-cost" ),
                             new BigRational( baseCost ) ) )
            .make();
}

TileType::TileType(Sise::SExp* sexp) {
    using namespace Sise;
    Cons *args = asProperCons( sexp );
    Cons *alist = asProperCons( args->getcdr() );
    std::string sym;

    symbol = *asSymbol( args->getcar() );

    name = *asString( alist->alistGet( "name" ) );

    sym = *asSymbol( alist->alistGet( "mobility" ) );
    if( sym == "wall" ) {
        mobility = Type::WALL;
    } else if( sym == "floor" ) {
        mobility = Type::FLOOR;
    } else throw std::logic_error( "invalid mobility type" );
    sym = *asSymbol( alist->alistGet( "opacity" ) );
    if( sym == "clear" ) {
        opacity = Type::CLEAR;
    } else if( sym == "block" ){
        opacity = Type::BLOCK;
    } else throw std::logic_error( "invalid opacity type" );
    sym = *asSymbol( alist->alistGet( "border?" ) );
    if( sym == "yes" ) {
        border = true;
    } else if( sym == "no" ){
        border = false;
    } else throw std::logic_error( "invalid border? type" );

    if( mobility != Type::WALL ) {
        baseCost = asMPQ( alist->alistGet( "base-cost" ) );
    }
}

Sise::SExp *UnitType::toSexp(void) const {
    using namespace Sise;
    return List()( new Symbol( symbol ) )
                 ( new Cons( new Symbol( "name" ),
                             new String( name ) ) )
                 ( new Cons( new Symbol( "max-hp" ),
                             new Int( maxHp ) ) )
                 ( new Cons( new Symbol( "speed" ),
                             new BigRational( speed ) ) )
                 ( new Cons( new Symbol( "native-layer" ),
                             new Int( nativeLayer ) ) )
            .make();
}

bool TileType::mayTraverse(const UnitType& unitType) const {
    mpq_class disregardThat;
    return mayTraverse( unitType, disregardThat );
}

bool TileType::mayTraverse(const UnitType& unitType, mpq_class& outCost) const {
    if( border ) return false;
    if( mobility == Type::WALL ) return false;
    outCost = baseCost;
    return true;
}

UnitType::~UnitType(void) {
    if( meleeAttack ) {
        delete meleeAttack;
    }
}

Sise::SExp* DefenseCapability::toSexp(void) const {
    using namespace Sise;
    using namespace SProto;
    return List()( new Int( defense ) )
                 ( new Int( reduction ) )
                 ( new BigRational( resistance ) )
           .make();
}
DefenseCapability::DefenseCapability(Sise::SExp* sexp) {
    using namespace Sise;
    using namespace SProto;
    Cons *args = asProperCons( sexp );

    defense = *asInt( args->nthcar(0) );
    reduction = *asInt( args->nthcar(1) );
    resistance = asMPQ( args->nthcar(2) );
}

Sise::SExp* AttackCapability::toSexp(void) const {
    using namespace Sise;
    using namespace SProto;
    return List()( new Int( attack ) )
                 ( new Int( shots ) )
                 ( new Int( firepower ) )
           .make();
}
AttackCapability::AttackCapability(Sise::SExp* sexp) {
    using namespace Sise;
    using namespace SProto;
    Cons *args = asProperCons( sexp );

    attack = *asInt( args->nthcar(0) );
    shots = *asInt( args->nthcar(1) );
    firepower = *asInt( args->nthcar(2) );
}


};
