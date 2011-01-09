#include "SiSExp.h"

#include <stdexcept>
#include <cassert>

namespace SiSExp {

SExp::SExp(Type) :
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

Cons::~Cons(void) {
    if( carPtr ) {
        carPtr->decref();
    }
    if( cdrPtr ) {
        cdrPtr->decref();
    }
}

};
