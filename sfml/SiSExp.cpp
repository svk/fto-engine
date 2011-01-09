#include "SiSExp.h"

#include <stdexcept>
#include <cassert>

#include <iostream>

#include <cstring>
#include <cstdlib>
#include <cstdio>

namespace SiSExp {

SExp::SExp(Type type) :
    type ( type ),
    refcount ( 0 )
{
}

SExp::~SExp(void) {
}

void SExp::decref(void) {
    assert( refcount > 0 );
    if( !--refcount ) {
        delete this;
    }
}

void SExp::incref(void) {
    ++refcount;
}

Cons* SExp::asCons(void) {
    const Type t = TYPE_CONS;
    if( !isType( t ) ) throw SExpTypeError( t, type );
    return dynamic_cast<Cons*>( this );
}

SExpTypeError::SExpTypeError(Type expected, Type got) :
    expected (expected),
    got (got)
{
}

bool SExp::isType(Type t) const {
    return type == t;
}

void Cons::setcar(SExp *car) {
    if( car ) {
        car->incref();
    }
    if( carPtr ) {
        carPtr->decref();
    }
    carPtr = car;
}

void Cons::setcdr(SExp *cdr) {
    if( cdr ) {
        cdr->incref();
    }
    cdrPtr = cdr;
}

Cons::Cons(SExp *car, SExp *cdr) :
    SExp( TYPE_CONS ),
    carPtr( 0 ),
    cdrPtr( 0 )
{
    setcar( car );
    setcdr( cdr );
}

Cons::Cons(SExp *car) :
    SExp( TYPE_CONS ),
    carPtr( 0 ),
    cdrPtr( 0 )
{
    setcar( car );
}

Cons::Cons(void) :
    SExp( TYPE_CONS ),
    carPtr( 0 ),
    cdrPtr( 0 )
{
}

Cons::~Cons(void) {
    if( carPtr ) {
        carPtr->decref();
    }
    if( cdrPtr ) {
        cdrPtr->decref();
    }
}

void Cons::output(std::ostream& os) {
    Cons *c = this;
    os.put( '(' );
    while( c ) {
        if( !c->carPtr ) {
            os.write( "NIL", 3 );
        } else {
            c->carPtr->output( os );
        }
        if( !c->cdrPtr ) {
            c = 0;
        } else if( c->cdrPtr->isType( TYPE_CONS ) ){
            os.put( ' ' );
            c = c->cdrPtr->asCons();
        } else {
            os.put( ' ' );
            os.put( '.' );
            os.put( ' ' );
            c->cdrPtr->output( os );
        }
    }
    os.put( ')' );
}

void Int::output(std::ostream& os) {
    char buffer[512];
    snprintf( buffer, sizeof buffer, "%d", data );
    os.write( buffer, strlen( buffer ) );
}

void String::output(std::ostream& os) {
    int sz = data.size();
    os.put( '"' );
    for(int i=0;i<sz;i++) switch( data[i] ) {
        case '\\':
        case '"':
            os.put( '\\' );
            // fallthrough to normal print
        default:
            os.put( data[i] );
    }
    os.put( '"' );
}

};
