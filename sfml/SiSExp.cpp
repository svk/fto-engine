#include "SiSExp.h"

#include <stdexcept>
#include <cassert>

#include <iostream>

#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <cctype>

namespace SiSExp {

SExp::SExp(Type type) :
    type ( type )
{
}

SExp::~SExp(void) {
}

Int* SExp::asInt(void) {
    const Type t = TYPE_INT;
    if( !isType( t ) ) throw SExpTypeError( t, type );
    return dynamic_cast<Int*>( this );
}

Symbol* SExp::asSymbol(void) {
    const Type t = TYPE_SYMBOL;
    if( !isType( t ) ) throw SExpTypeError( t, type );
    return dynamic_cast<Symbol*>( this );
}

String* SExp::asString(void) {
    const Type t = TYPE_STRING;
    if( !isType( t ) ) throw SExpTypeError( t, type );
    return dynamic_cast<String*>( this );
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
    if( carPtr ) {
        delete carPtr;
    }
    carPtr = car;
}

void Cons::setcdr(SExp *cdr) {
    if( cdrPtr ) {
        delete cdrPtr;
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
        delete carPtr;
    }
    if( cdrPtr ) {
        delete cdrPtr;
    }
}

void Cons::output(std::ostream& os) {
    Cons *c = this;
    os.put( '(' );
    while( c ) {
        outputSExp( c->carPtr, os );
        if( !c->cdrPtr ) {
            c = 0;
        } else if( c->cdrPtr->isType( TYPE_CONS ) ){
            os.put( ' ' );
            c = c->cdrPtr->asCons();
        } else {
            os.put( ' ' );
            os.put( '.' );
            os.put( ' ' );
            outputSExp( c->cdrPtr, os );
            c = 0;
        }
    }
    os.put( ')' );
}

void Int::output(std::ostream& os) {
    char buffer[512];
    snprintf( buffer, sizeof buffer, "%d", data );
    os.write( buffer, strlen( buffer ) );
}

void Symbol::output(std::ostream& os) {
    int sz = data.size();
    // these are simple symbols -- no quoting. all chars assumed legal without such
    os.write( data.data(), sz );
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

SExp* SymbolParser::get(void) {
    return new Symbol( oss.str() );
}

SExp* StringParser::get(void) {
    return new String( oss.str() );
}

bool SymbolParser::done(void) const {
    return isDone;
}


bool StringParser::done(void) const {
    return isDone;
}

bool SymbolParser::feed(char ch) {
    if( isspace( ch ) ) {
        isDone = true;
    } else if( ch == ')' ) {
        isDone = true;
        return false;
    } else if( !isprint( ch ) ) {
        throw ParseError( "unexpected char in symbol" );
    } else {
        oss << ch;
    }
    return true;
}

bool StringParser::feed(char ch) {
    if( quoted ) {
        oss << ch;
    } else if( ch == '\\' ) {
        quoted = true;
    } else if( ch == '"' ) {
        isDone = true;
    } else {
        oss << ch;
    }
    return true;
}

SExp* NumberParser::get(void) {
    // no floats for now
    int rv = atoi( buffer );
    return new Int( rv );
}

bool NumberParser::done(void) const {
    return isDone;
}

bool NumberParser::feed(char ch) {
    if( length >= (int) sizeof buffer ) {
        throw ParseError( "parse error / buffer overflow -- expected smallint" );
    }
    if( isdigit( ch ) ) {
        buffer[length++] = ch;
    } else {
        isDone = true;
        return false;
    }
    return true;
}

NumberParser::NumberParser(void) :
    length ( 0 ),
    isDone ( false )
{
    memset( buffer, 0, sizeof buffer );
}

SExpParser *makeSExpParser(char ch) {
    SExpParser *rv = 0;
    if( isspace( ch ) ) {
        return 0;
    } else if( isdigit( ch ) ) {
        rv = new NumberParser();
        rv->feed( ch );
    } else if( ch == '"' ) {
        rv = new StringParser();
    } else if( ch == '(' ) {
        rv = new ListParser();
    } else if( isprint( ch ) ) {
        rv = new SymbolParser();
        rv->feed( ch );
    } else {
        throw ParseError( "oh noes -- I am just a few commits old and what is this" );
    }
    return rv;
}

bool ListParser::done(void) const {
    return phase == DONE;
}

SExp* ListParser::get(void) {
    bool first = true;
    Cons *rv = 0;
    using namespace std;
    for(std::list<SExp*>::reverse_iterator i = elements.rbegin(); i != elements.rend(); i++) {
        if( first ) {
            rv = new Cons( *i, terminatingCdr );
            first = false;
        } else {
            rv = new Cons( *i, rv );
        }
    }
    return rv;
}

bool ListParser::feed(char ch) {
    bool took = false;
    if( subparser ) {
        took = subparser->feed( ch );
        if( subparser->done() ) {
            SExp *sexp = subparser->get();
            delete subparser;
            subparser = 0;
            switch( phase ) {
                case LIST_ITEMS:
                    elements.push_back( sexp );
                    break;
                case CDR_ITEM:
                    terminatingCdr = sexp;
                    phase = WAITING_FOR_TERMINATION;
                    break;
                default:
                    throw ParseError( "parse or internal error -- unexpected atom" );
            }
        }
    }
    if( !took && !isspace( ch ) ) switch( ch ) {
        case ')':
            if( phase == LIST_ITEMS || phase == WAITING_FOR_TERMINATION ) {
                phase = DONE;
                break;
            }
            throw ParseError( "parse error -- unexpected end of cons" );
        case '.':
            if( phase == LIST_ITEMS ) {
                phase = CDR_ITEM;
                break;
            }
            throw ParseError( "parse error -- unexpected dot in cons" );
        default:
            subparser = makeSExpParser( ch );
            break;
    }
    return true;
}

ListParser::ListParser(void) :
    phase ( LIST_ITEMS ),
    elements (),
    terminatingCdr ( 0 ),
    subparser ( 0 )
{
}

SymbolParser::SymbolParser(void) :
    isDone ( false ),
    oss ()
{
}

StringParser::StringParser(void) :
    quoted ( false ),
    isDone ( false ),
    oss ()
{
}

ListParser::~ListParser(void) {
}

SExp *SExpStreamParser::pop(void) {
    SExp *rv = rvs.front();
    rvs.pop();
    return rv;
}

bool SExpStreamParser::empty(void) const {
    return rvs.empty();
}

SExpStreamParser::SExpStreamParser(void) :
    rvs (),
    parser ( 0 )
{
}

SExpStreamParser::~SExpStreamParser(void) {
    // freeing partially parsed stuff on shutdown
    while( !rvs.empty() ) {
        delete rvs.front();
        rvs.pop();
    }
    delete parser;
}

void SExpStreamParser::feed(char ch) {
    if( parser ) {
        parser->feed( ch );
        if( parser->done() ) {
            using namespace std;
            rvs.push( parser->get() );
            delete parser;
            parser = 0;
        }
    } else {
        parser = makeSExpParser( ch );
    }
}

void outputSExp(SExp* sexp, std::ostream& os) {
    if( sexp ) {
        sexp->output( os );
    } else {
        os.put( '(' );
        os.put( ')' );
    }
}

SExp *Cons::getcar(void) const {
    return carPtr;
}

SExp *Cons::getcdr(void) const {
    return cdrPtr;
}

void SExpStreamParser::end(void) {
    if( parser ) {
        parser->feedEnd();
        if( parser->done() ) {
            rvs.push( parser->get() );
            delete parser;
            parser = 0;
        }
    }
}

};
