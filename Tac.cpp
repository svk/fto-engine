#include "Tac.h"

namespace Tac {

UnitType::UnitType(const std::string& symbol, const std::string& name, int speed, int maxHp) :
    symbol ( symbol ),
    name ( name ),
    maxHp ( maxHp ),
    speed ( speed ),
    nativeLayer ( 0 ) // xx
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
    using namespace std;
    outputSExp( alist, cerr );
    name = *asString( alist->alistGet( "name" ) );
    maxHp = *asInt( alist->alistGet( "max-hp" ) );
    speed = *asInt( alist->alistGet( "speed" ) );
    nativeLayer = *asInt( alist->alistGet( "native-layer" ) );
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
                             new Int( baseCost ) ) )
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
        baseCost = *asInt( alist->alistGet( "base-cost" ) );
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
                             new Int( speed ) ) )
                 ( new Cons( new Symbol( "native-layer" ),
                             new Int( nativeLayer ) ) )
            .make();
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
