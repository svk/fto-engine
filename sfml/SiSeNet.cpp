#include "SiSeNet.h"

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

namespace SiSeNet {

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
    gcShutdownMode ( false )
{
}

OutputBuffer::OutputBuffer(void) :
    capacity ( INITIAL_BUFFER_CAPACITY ),
    data ( new char [ capacity ] )
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
        instream.feed( buffer[i] );
    }
}

std::ostream& Socket::out(void) {
    return outstream;
}

SiSExp::SExpStreamParser& Socket::in(void) {
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
    if( rv == EINTR ) return;
    if( rv < 0 ) {
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
            } catch( SiSExp::ParseError& perr ) {
                using namespace std;
                cerr << perr.what() << endl;
                fatalParseError = true;
            }
        }
        if( fatalParseError ||
            i->second->hasFatalError() ) {
            using namespace std;
            cerr << "fatal (parse?) error oops" << endl;
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

SocketManager::~SocketManager(void) {
    for(WatchedMap::iterator i = watched.begin(); i != watched.end();i++) {
        delete i->second;
    }
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
    struct addrinfo hints, *res;

    char ports[512];
    snprintf( ports, sizeof ports, "%d", port );

        // TODO error handling
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo( addr.c_str(), ports, &hints, &res );

    using namespace std;

    RawSocket x = socket( res->ai_family, res->ai_socktype, res->ai_protocol );
    if( x == INVALID_SOCKET ) {
        throw std::runtime_error( "unable to create outgoing connection socket" );
    }

    connect( x, res->ai_addr, res->ai_addrlen );

    return new Socket( x );
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

}

