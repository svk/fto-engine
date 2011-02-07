#include "Sise.h"

#include "boost/filesystem.hpp"

#include <stdexcept>
#include <cassert>

#include <iostream>

#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <cctype>

#define MAX_SEND_SIZE 4096
#define INITIAL_BUFFER_CAPACITY 1024
#define LISTEN_BACKLOG 5
#define INPUT_BUFFER_SIZE 1024

#include <stdexcept>

#include <cstring>
#include <cstdio>

#include <cerrno>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

namespace Sise {

SExp::SExp(Type type) :
    type ( type )
{
}

SExp::~SExp(void) {
}

Int* asInt(SExp* x) {
    const Type t = TYPE_INT;
    if( !x ) throw UnexpectedNilError();
    if( !x->isType( t ) ) throw SExpTypeError( t, x->getType() );
    return dynamic_cast<Int*>( x );
}

Symbol* asSymbol(SExp* x) {
    const Type t = TYPE_SYMBOL;
    if( !x ) throw UnexpectedNilError();
    if( !x->isType( t ) ) throw SExpTypeError( t, x->getType() );
    return dynamic_cast<Symbol*>( x );
}

String* asString(SExp *x) {
    const Type t = TYPE_STRING;
    if( !x ) throw UnexpectedNilError();
    if( !x->isType( t ) ) throw SExpTypeError( t, x->getType() );
    return dynamic_cast<String*>( x );
}

Cons* asProperCons(SExp* x) {
    Cons *rv = asCons( x );
    if( !rv ) throw UnexpectedNilError();
    return rv;
}

Cons* asCons(SExp* x) {
    const Type t = TYPE_CONS;
    if( !x ) return 0; // nil is a valid cons
    if( !x->isType( t ) ) throw SExpTypeError( t, x->getType() );
    return dynamic_cast<Cons*>( x );
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
        outputSExp( c->carPtr, os, false );
        if( !c->cdrPtr ) {
            c = 0;
        } else if( c->cdrPtr->isType( TYPE_CONS ) ){
            os.put( ' ' );
            c = asCons( c->cdrPtr );
        } else {
            os.put( ' ' );
            os.put( '.' );
            os.put( ' ' );
            outputSExp( c->cdrPtr, os, false );
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
        quoted = false;
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
    if( isdigit( ch ) || (length == 0 && ch == '-') ) {
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
    } else if( isdigit( ch ) || ch == '-' ) {
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
    elements.clear();
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
    for(std::list<SExp*>::iterator i = elements.begin(); i != elements.end(); i++) {
        if( *i ) {
            delete *i;
        }
    }
    if( terminatingCdr ) {
        delete terminatingCdr;
    }
    if( subparser ) {
        delete subparser;
    }
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
        SExp *sexp = rvs.front();
        if( sexp ) {
            delete sexp;
        }
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

void outputSExp(SExp* sexp, std::ostream& os, bool terminateWithWhitespace) {
    if( sexp ) {
        sexp->output( os );
        if( terminateWithWhitespace ) {
            os.put( '\n' );
        }
    } else {
        os.put( '(' );
        os.put( ')' );
        if( terminateWithWhitespace ) {
            os.put( '\n' );
        }
    }
}

SExp *Cons::getcar(void) const {
    assert( this );
    return carPtr;
}

SExp *Cons::getcdr(void) const {
    assert( this );
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

SExp* Cons::nthcar(int n) {
    Cons *c = asCons( nthtail(n) );
    if( !c ) {
        throw UnexpectedNilError();
    }
    return c->getcar();
}

SExp* Cons::nthtail(int n) {
    assert( n >= 0 );
    if( n == 0 ) return this;
    if( n == 1 ) return cdrPtr;
    if( !cdrPtr ) {
        throw UnexpectedNilError();
    }
    Cons *c = asCons( cdrPtr );
    return c->nthtail( n - 1 );
}

void Socket::gracefulShutdown(void) {
    shutdown( sock, 1 );
    gcShutdownMode = true;
}

Socket::~Socket(void) {
    closesocket( sock );
}

Socket::Socket(RawSocket sock) :
    sock ( sock ),
    outbuffer (),
    instream (),
    outstream ( &outbuffer ),
    errorstate ( false ),
    gcShutdownMode ( false ),
    doSpyInput ( false )
{
}

OutputBuffer::OutputBuffer(void) :
    capacity ( INITIAL_BUFFER_CAPACITY ),
    data ( new char [ capacity ] ),
    spy ( false )
{
    setp( data, data + capacity - 1 );
}

std::string OutputBuffer::debugGetString(void) {
    return std::string( pbase(), pptr() - pbase() );
}

void OutputBuffer::consume(int n) {
    memmove( data, &data[n], getSize() - n );
    pbump( -n );
}

int OutputBuffer::overflow(int c) {
    using namespace std;
    int sz = pptr() - pbase();
    if( sz == capacity ) {
        capacity *= 2;
        char * nd = new char [ capacity ];
        memcpy( nd, data, sz );
        setp( nd, nd + capacity - 1 );
        pbump( sz );
        delete [] data;
        data = nd;
    }
    if( c != EOF ) {
        *pptr() = c;
        pbump(1);
    }
    return 0;
}

OutputBuffer::~OutputBuffer(void) {
    delete [] data;
}

int OutputBuffer::getSize(void) {
    return pptr() - pbase();
}

bool OutputBuffer::tryFlushToSocket(RawSocket sock) {
    int size = getSize();
    if( size < 1 ) return true;
    int rv = send( sock, data, MIN( MAX_SEND_SIZE, size ) , 0 );
    int sent = 0;
    using namespace std;
    while( rv == MAX_SEND_SIZE ) {
        sent += MAX_SEND_SIZE;
        rv = send( sock, &data[sent], MIN( MAX_SEND_SIZE, size - sent ), 0 );
    }
    if( rv > 0 ) {
        sent += rv;
    }
    if( spy ) {
        using namespace std;
        cerr << std::string( data, sent );
    }
    consume( sent );
    return (rv >= 0);
}

void Socket::transmit(void) {
    if( errorstate ) return;
    if( !outbuffer.tryFlushToSocket( sock ) ) {
        errorstate = true;
    }
}

void Socket::receive(void) {
    using namespace std;
    char buffer[INPUT_BUFFER_SIZE];
    if( errorstate ) return;
    int rv = recv( sock, buffer, sizeof buffer, 0 );
    if( rv <= 0 ) {
        errorstate = true;
    } else if( !gcShutdownMode ) for(int i=0;i<rv;i++) {
        if( doSpyInput ) {
            cerr << buffer[i];
        }
        instream.feed( buffer[i] );
    }
}

std::ostream& Socket::out(void) {
    return outstream;
}

SExpStreamParser& Socket::in(void) {
    return instream;
}

RawSocket Socket::getSocket(void) {
    return sock;
}

void SocketManager::watch( Socket *s ) {
    int n = s->getSocket();
    watched[ n ] = s;
    FD_SET( n, &fullWatched );
    maxWatched = MAX( n, maxWatched );
}

void SocketManager::unwatch( Socket *s, bool noErase ) {
    int n = s->getSocket();
    FD_CLR( n, &fullWatched );
    maxWatched = 0;
    for(WatchedMap::iterator i = watched.begin(); i != watched.end();) {
        if( i->first == n ) {
            if( !noErase ) {
                WatchedMap::iterator j = i;
                i++;
                watched.erase( j );
            }
        } else {
            maxWatched = MAX( i->first, maxWatched );
            i++;
        }
    }
}

void SocketManager::pump(int ms, SocketSet* ready) {
    fd_set readfds = fullWatched,
           exceptfds = fullWatched;
    struct timeval tv = { ms / 1000, (ms%1000) * 1000 };
    using namespace std;
    for(WatchedMap::iterator i = watched.begin(); i != watched.end();i++) {
        i->second->transmit();
    }
    int rv = select( MAX( maxWatched, maxListener ) + 1,
                     &readfds,
                     0,
                     &exceptfds,
                     (ms < 0) ? 0 : &tv );
    if( rv < 0 ) {
        if( errno == EINTR ) return;
        using namespace std;
        cerr << rv << endl;
        throw std::runtime_error( "select() returned unexpected error" );
    }
    for(WatchedMap::iterator i = watched.begin(); i != watched.end();) {
        bool fatalParseError = false;
        if( FD_ISSET( i->first, &readfds ) ||
            FD_ISSET( i->first, &exceptfds ) ) {
            try {
                i->second->receive();
                if( ready && !i->second->in().empty() ) {
                    ready->insert( i->second );
                }
            } catch( ParseError& perr ) {
                using namespace std;
                cerr << perr.what() << endl;
                fatalParseError = true;
            }
        }
        if( fatalParseError ||
            i->second->hasFatalError() ) {
            using namespace std;
            WatchedMap::iterator j = i;
            i++;
            Socket *deletable = j->second;
            watched.erase( j );
            unwatch( deletable, true );
            delete deletable;
        } else {
            i++;
        }
    }
    for(std::vector<RawSocket>::iterator i = listeners.begin(); i != listeners.end();i++) {
        if( FD_ISSET( *i, &readfds ) ) {
            struct sockaddr_storage their_addr;
            socklen_t addr_size = sizeof their_addr;
            RawSocket ns = accept( *i,
                                   reinterpret_cast<struct sockaddr*>(&their_addr),
                                   &addr_size );
            Socket * nsp = 0;
            if( greeter ) {
                nsp = greeter->greet( ns, &their_addr, addr_size );
            } else {
                nsp = new Socket( ns );
            }
            if( nsp ) {
                adopt( nsp );
            }
        }
    }
    if( doCheckStdin ) {
        stdinFlag = FD_ISSET( fileno(stdin), &readfds );
    }
}

void SocketManager::unwatchAll(void) {
    for(WatchedMap::iterator i = watched.begin(); i != watched.end();i++) {
        delete i->second;
    }
    watched.clear();
}

SocketManager::~SocketManager(void) {
    unwatchAll();
    for(std::vector<RawSocket>::iterator i = listeners.begin(); i != listeners.end();i++) {
        closesocket( *i );
    }
}

void SocketManager::adopt( Socket* socket ) {
    watch( socket );
}

bool Socket::hasFatalError(void) const {
    return errorstate;
}

void SocketManager::setGreeter(SocketGreeter* g) {
    greeter = g;
}

bool SocketManager::addListener( int port ) {
    struct sockaddr_in sa;
    RawSocket sock = socket( AF_INET, SOCK_STREAM, 0 );
    if( sock == INVALID_SOCKET ) {
        throw std::runtime_error( "unable to create listener socket" );
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons( port );
    sa.sin_addr.s_addr = INADDR_ANY;


    // this is non-critical; just attempt it.
    int yes = 1;
    setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes );

    int rv = bind( sock, (struct sockaddr*) &sa, sizeof sa);
    if( rv == -1 ) {
        throw std::runtime_error( "bind failed" );
    }

    rv = listen( sock, LISTEN_BACKLOG );
    if( rv == -1 ) {
        throw std::runtime_error( "listen failed" );
    }

    listeners.push_back( sock );
    maxListener = MAX( maxListener, sock );
    FD_SET( sock, &fullWatched );
    return true;
}

bool Socket::wouldTransmit(void) const {
    return outbuffer.hasWaiting();
}

bool OutputBuffer::hasWaiting(void) const {
    return pptr() != data;
}

Socket *Socket::connectTo(const std::string& addr, int port) {
    return connectToAs<Socket>( addr, port );
}

SocketManager::SocketManager(void) :
    greeter ( 0 ),
    listeners (),
    fullWatched (),
    watched (),
    maxWatched ( 0 ),
    maxListener ( 0 ),
    doCheckStdin ( false ),
    stdinFlag ( false )
{
    FD_ZERO( &fullWatched );
}

void SocketManager::checkStdin(void) {
    FD_SET( fileno(stdin), &fullWatched );
    doCheckStdin = true;
}

bool SocketManager::stdinFlagged(void) const {
    return stdinFlag;
}

SocketManager::SocketSet SocketManager::debugGetSockets(void) {
    SocketSet rv;
    for(WatchedMap::iterator i = watched.begin(); i != watched.end();i++) {
        rv.insert( i->second );
    }
    return rv;
}

int SocketManager::numberOfWatchedSockets(void) {
    return watched.size();
}

void ConsSocket::pump(void) {
    while( !in().empty() ) {
        SExp *sexp = in().pop();
        using namespace std;
        try {
            Cons *cons = asCons( sexp );
            Symbol *sym = asSymbol( cons->getcar() );
            handle( sym->get(), cons->getcdr() );
            delete sexp;
        }
        catch(...) {
            delete sexp;
            throw;
        }
    }
}

void ConsSocketManager::manage(int ms) {
    SocketSet socks;
    pump( ms, &socks );
    std::vector<ConsSocket*> errorsocks;
    for(SocketSet::iterator i = socks.begin(); i != socks.end(); i++) {
        try {
            dynamic_cast<ConsSocket*>((*i))->pump();
        }
        catch( Sise::SExpInterpretationError& e ) {
            using namespace std;
            cerr << "warning: error (" << e.what() << ") on socket, closing" << endl;
            errorsocks.push_back( dynamic_cast<ConsSocket*>(*i) );
        }
    }
    for(std::vector<ConsSocket*>::iterator i = errorsocks.begin(); i != errorsocks.end(); i++) {
        using namespace std;
        unwatch( *i, false );
        delete *i;
    }
}

std::string getAddressString( struct sockaddr_storage *st, socklen_t sz ) {
    char buffer[512];
    switch( st->ss_family ) {
        case AF_INET:
            {
                struct sockaddr_in *in = reinterpret_cast<struct sockaddr_in*>( st );
                if( sz < sizeof *in ) break;
                inet_ntop( st->ss_family, &in->sin_addr, buffer, sizeof buffer );
                return buffer;
            }
        case AF_INET6:
            {
                struct sockaddr_in6 *in = reinterpret_cast<struct sockaddr_in6*>( st );
                if( sz < sizeof *in ) break;
                inet_ntop( st->ss_family, &in->sin6_addr, buffer, sizeof buffer );
                return buffer;
            }
    }
    throw std::runtime_error( "unknown or corrupt sockaddr_storage" );
}

int getPort( struct sockaddr_storage *st, socklen_t sz ) {
    switch( st->ss_family ) {
        case AF_INET:
            {
                struct sockaddr_in *in = reinterpret_cast<struct sockaddr_in*>( st );
                if( sz < sizeof *in ) break;
                return ntohs( in->sin_port );
            }
        case AF_INET6:
            {
                struct sockaddr_in6 *in = reinterpret_cast<struct sockaddr_in6*>( st );
                if( sz < sizeof *in ) break;
                return ntohs( in->sin6_port );
            }
    }
    throw std::runtime_error( "unknown or corrupt sockaddr_storage" );
}

SExp * readSExpFromFile(const std::string& filename) {
    using namespace std;
    SExpStreamParser streamParser;
    ifstream is ( filename.c_str(), ios::in );
    char byte;
    if( !is.good() ) {
        throw FileInputError();
    }
    while( !is.eof() ) {
        is.read( &byte, 1 );
        streamParser.feed( byte );
    }
    streamParser.end();
    if( streamParser.empty() ) throw FileInputError();
    return streamParser.pop();
}

bool writeSExpToFile(const std::string& filename, SExp *sexp) {
    using namespace std;
    ofstream os ( filename.c_str(), ios::out );
    outputSExp( sexp, os, true );
    return os.good();
}

void readSExpDir( const std::string& dirname, const std::string& ext, NamedSexpHandler& nsh ) {
    namespace fs = boost::filesystem;
    fs::path path = fs::system_complete( dirname );
    if( !fs::exists( path ) || !fs::is_directory( path ) ) {
        throw std::runtime_error( "readSExpDir called on nonexistent file or non-directory" );
    }
    fs::directory_iterator end_iter;
    for( fs::directory_iterator i ( path ); i != end_iter; i++) {
        using namespace std;
        if( fs::is_regular_file( i->status() ) && i->path().extension() == ext ) {
            try {
                SExp *rv = readSExpFromFile( i->path().string() );
                nsh.handleNamedSExp( i->path().filename(), rv );
                delete rv;
            }
            catch( FileInputError& e ) {
                using namespace std;
                cerr << "warning: input error on " << i->path() << endl;
            }
        }
    }
}

void removeAllFilesWithExtension( const std::string& dirname, const std::string& ext ) {
    namespace fs = boost::filesystem;
    fs::path path = fs::system_complete( dirname );
    if( !fs::exists( path ) || !fs::is_directory( path ) ) {
        throw std::runtime_error( "readSExpDir called on nonexistent file or non-directory" );
    }
    fs::directory_iterator end_iter;
    for( fs::directory_iterator i ( path ); i != end_iter; i++) {
        if( fs::is_regular_file( i->status() ) && i->path().extension() == ext ) {
            fs::remove( i->path() );
        }
    }
}

void OutputBuffer::debugSetSpy(bool spy_) {
    spy = spy_;
}

void Socket::debugSetInputSpy(bool spy_){
    doSpyInput = spy_;
}

void Socket::debugSetOutputSpy(bool spy_){
    outbuffer.debugSetSpy( spy_ );
}

SExp *Cons::alistGet(const std::string& key) {
    Cons *current = this;
    while( current ) {
        Cons *candidate = asProperCons( current->getcar() );
        if( key == (std::string)*asSymbol(candidate->getcar()) ) {
            return candidate->getcdr();
        }
        current = asCons( current->getcdr() );
    }
    return 0;
}


}
