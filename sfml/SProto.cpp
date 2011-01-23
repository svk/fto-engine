#include "SProto.h"

#define PROTOCOL_ID "SProto"
#define PROTOCOL_VERSION 1
#define PROTOCOL_SERVER_ID "StdServer"
#define PROTOCOL_CLIENT_ID "StdClient"

#include <cassert>

#include <cctype>
#include <algorithm>

#include <iomanip>

#include <openssl/sha.h>

#define CHALLENGE_SALT "123456789abcdefghi"
#define PASSWORD_SALT  "abcdefghi123456789"

namespace SProto {

RemoteClient::RemoteClient(Sise::RawSocket sock,
                           Server& server,
                           const std::string& netId,
                           std::vector<RemoteClient*>& rclients ) :
    SProtoSocket ( sock ),
    server ( server ),
    state ( ST_SILENT ),
    netId ( netId ),
    username ( "" ),
    subserver ( 0 ),
    rclients ( rclients ),
    loggingIn ( false )
{
    rclients.push_back( this );
}

RemoteClient::~RemoteClient(void) {
    using namespace std;
    if( subserver ) {
        subserver->leaving( this );
    }
    rclients.erase( remove( rclients.begin(), rclients.end(), this ) );
}

void SProtoSocket::close(void) {
    closing = true;
    gracefulShutdown();
}

void RemoteClient::leave(void) {
    if( subserver ) {
        subserver->leaving( this );
        subserver = 0;
    }
}

void RemoteClient::handle( const std::string& cmd, Sise::SExp *arg ) {
    using namespace Sise;
    const bool strictMode = true;
    if( subserver ) {
        subserver->handle( this, cmd, arg );
        return;
    }
    if( cmd == "hello" ) {
        Cons *args = asProperCons( arg );
        std::string protoName = *asSymbol( args->nthcar(0) );
        int majorVersion = *asInt( args->nthcar(1) );
        std::string clientName = *asString( args->nthcar(2) );
        if( state != ST_SILENT ||
            protoName != PROTOCOL_ID ||
            majorVersion != PROTOCOL_VERSION ) {
            close();
        } else {
            state = ST_VERSION_OK;
            delsendPacket( "hello",
                List()( new String( PROTOCOL_SERVER_ID ) )
                .make() );
        }
    } else if( state == ST_SILENT && strictMode ) {
        close();
    } if( cmd == "login-request" ) {
        Cons *args = asProperCons( arg );
        loggingIn = true;
        desiredUsername = *asString( args->nthcar(0) );
        loginChallenge = server.makeChallenge();
        delsendPacket( "login-challenge",
                       List()( new String( desiredUsername ) )
                             ( new String( loginChallenge ) )
                       .make() );
    } else if( cmd == "check-username" ) {
        Cons *args = asProperCons( arg );
        std::string uname = *asString( args->nthcar(0) );
        delsendPacket( "check-username-response",
                       List()( new String(uname) )
                             ( new String( server.usernameAvailable(uname) ) )
                       .make() );
    } else if( cmd == "register" ) {
        Cons *args = asProperCons( arg );
        std::string uname = *asString( args->nthcar(0) );
        std::string pword = *asString( args->nthcar(1) );
        std::string failureReason = server.registerUsername( uname, pword );
        if( failureReason != "ok" ) {
            delsendPacket( "register-failure",
                           List()( new String( failureReason ))
                           .make() );
        } else {
            delsendPacket( "register-ok",
                           List()( new String( uname ) )
                           .make() );
        }

    } else if( cmd == "login-response" ) {
        Cons *args = asProperCons( arg );
        if( !loggingIn ) {
            close();
        } else {
            loggingIn = false;
            std::string response = *asString( args->nthcar(0) );
            try {
                if( response == server.solveChallenge( desiredUsername,
                                                       loginChallenge ) ) {
                    username = desiredUsername;
                    delsendPacket( "login-ok",
                                   List()( new String( desiredUsername ) )
                                   .make() );
                } else {
                    delsendPacket( "login-failure",
                                   List()( new String( "login failed" ))
                                   .make() );
                }
            }
            catch( NoSuchUserException& e ) {
                delsendPacket( "login-failure",
                               List()( new String( "login failed" ))
                               .make() );
            }
        }
    } else if( cmd == "select-server" ) {
        Cons *args = asProperCons( arg );
        std::string gameName = *asSymbol( args->nthcar(0) );
        subserver = server.getSubServer( gameName );
        if( !subserver || !subserver->entering( this ) ) {
            subserver = 0;
            close();
        }
    } else if( cmd == "goodbye" ) {
        close();
    } else if( cmd == "debug-hash" ) {
        Cons *args = asProperCons( arg );
        std::string data = *asString( args->nthcar(0) );
        delsendPacket( "debug-reply",
                       List()( new String( getHash( data ) ) )
                       .make() );
    } else if( cmd == "who-am-i" ) {
        delsendPacket( "debug-reply",
                       List()( new String( username ) )
                       .make() );
    } else if( cmd == "shutdown" ) {
        if( !server.getUsers().isAdministrator( username ) ) {
            delsendPacket( "permission-denied", 0 );
        } else {
            server.stopServer();
        }
    }
}

void SProtoSocket::delsendPacket( const std::string& name, Sise::SExp* cdr ) {
    using namespace Sise;
    delsend( new Cons( new Symbol( name ), cdr ) );
}

void SProtoSocket::delsend( Sise::SExp* sexp ) {
    if( !closing ) {
        Sise::outputSExp( sexp, out() );
    }
    delete sexp;
}

SubServer* Server::getSubServer(const std::string& s) {
    SubserverMap::iterator i = subservers.find( s );
    if( i == subservers.end() ) {
        return 0;
    }
    return i->second;
}

void Server::setSubServer(const std::string& name, SubServer* subserv) {
    subservers[ name ] = subserv;
}

Server::Server(void) :
    ConsSocketManager (),
    rclients (),
    subservers (),
    running ( true ),
    users ()
{
    setGreeter( this );

    users.restore();
}

Sise::Socket* Server::greet(Sise::RawSocket sock, struct sockaddr_storage *addr, socklen_t len) {
    std::ostringstream oss;
    oss << Sise::getAddressString( addr, len ) << ":" << Sise::getPort( addr, len );
    RemoteClient *rv = new RemoteClient( sock, *this, oss.str(), rclients );
    return rv;
}

std::string getHash(const std::string& data) {
    unsigned char rawhash[SHA256_DIGEST_LENGTH];
    std::ostringstream oss;
    SHA256_CTX ctx;
    oss << std::setfill('0') <<  std::hex << std::right << std::setw(2);
    SHA256_Init( &ctx );
    SHA256_Update( &ctx, data.data(), data.length() );
    SHA256_Final( rawhash, &ctx );
    for(int i=0;i<SHA256_DIGEST_LENGTH;i++) {
        oss << (int) rawhash[i];
    }
    return oss.str();
}

std::string makeChallengeResponse( const std::string& username, const std::string& passwordhash, const std::string& challenge ) {
    std::ostringstream oss;
    oss << CHALLENGE_SALT << username << passwordhash << challenge;
    return getHash( oss.str() );
}

std::string makePasswordHash( const std::string& username, const std::string& password ) {
    std::ostringstream oss;
    oss << PASSWORD_SALT << username << password;
    return getHash( oss.str() );
}

SProtoSocket::SProtoSocket( Sise::RawSocket dob ) :
    Sise::ConsSocket( dob ),
    closing ( false )
{
}

void Server::stopServer(void) {
    running = false;
}

Server::~Server(void) {
    unwatchAll();

    users.save();
}

void Client::setCore(ClientCore *core) {
    clientCore = core;
}

Client::Client( Sise::RawSocket sock, ClientCore* core ) :
    SProtoSocket( sock ),
    idState( IDST_UNIDENTIFIED ),
    clientCore ( core )
{
    using namespace Sise;
    delsendPacket( "hello",
                   List()( new Symbol( PROTOCOL_ID ) )
                         ( new Int( PROTOCOL_VERSION ) )
                         ( new String( PROTOCOL_CLIENT_ID ) )
                   .make() );
}

void Client::handle( const std::string& cmd, Sise::SExp *arg) {
    using namespace Sise;
    if( cmd == "echo" ) {
        Cons *args = asProperCons( arg );
        std::string data = *asString( args->nthcar(0) );
        delsendPacket( "echo-reply",
                       List()( new String( data ) )
                       .make() );
    } else if( cmd == "login-challenge" ) {
        Cons *args = asProperCons( arg );
        std::string data = *asString( args->nthcar(1) );

        assert( idState == IDST_IDENTIFYING );
        std::string response = makeChallengeResponse( username, passwordhash, data );

        delsendPacket( "login-response",
                       List()( new String( response ) )
                       .make() );
    } else if( cmd == "login-ok" ) {
        Cons *args = asProperCons( arg );
        std::string data = *asString( args->nthcar(0) );
        username = data;

        idState = IDST_IDENTIFIED;
    } else if (clientCore) {
        clientCore->handle( cmd, arg );
    }
}

void Client::identify(const std::string& username_, const std::string& password) {
    using namespace Sise;
    username = username_;
    passwordhash = makePasswordHash( username, password );
    delsendPacket( "login-request",
                   List()( new String( username ) )
                   .make() );
    idState = IDST_IDENTIFYING;
}

std::string Server::makeChallenge(void) {
    std::ostringstream oss;
    for(int i=0;i<64;i++) {
        oss << (char) ('a' + rand()%26);
    }
    return oss.str();
}

std::string Server::usernameAvailable(const std::string& username) {
    return users.usernameAvailable( username );
}

std::string Server::registerUsername( const std::string& username, const std::string& password ) {
    return users.registerUsername( username, password );
}

std::string Server::solveChallenge( const std::string& username, const std::string& challenge ) {
    return users.solveChallenge( username, challenge );
}

std::string UsersInfo::usernameAvailable(const std::string& username) {
    if( username.length() < 3 ) return "illegal";
    if( !isalpha( username[0] ) ) return "illegal";
    for(int i=0;i<(int)username.length();i++) {
        if( !isalnum( username[i] ) ) return "illegal";
    }
    if( users.find( username ) != users.end() ) {
        return "taken";
    }
    return "ok";
}

std::string UsersInfo::registerUsername( const std::string& username, const std::string& password ) {
    std::string reason = usernameAvailable( username );
    if( reason == "ok" ) {
        users[ username ] = UserInfo( makePasswordHash( username, password ) );
    }
    return reason;
}

std::string UsersInfo::solveChallenge( const std::string& username, const std::string& challenge ) {
    UsersMap::iterator i = users.find( username );
    if( i == users.end() ) {
        throw NoSuchUserException();
    }
    std::string passwordhash = i->second.passwordhash;
    return makeChallengeResponse( username, passwordhash, challenge );
}

UsersInfo::UserInfo::UserInfo( const std::string& passwordhash, bool admin ) :
    passwordhash ( passwordhash ),
    isAdmin ( admin )
{
}

Sise::SExp* UsersInfo::toSexp(void) const {
    using namespace Sise;
    List l;
    for(UsersMap::const_iterator i = users.begin(); i != users.end(); i++) {
        l( List()( new String( i->first ) )
                 ( new String( i->second.passwordhash ) )
                 ( new Symbol( i->second.isAdmin ? "admin" : "user" ) )
           .make() );
    }
    return l.make();
}

void UsersInfo::fromSexp(Sise::SExp* sexp) {
    using namespace Sise;
    Cons *l = asCons( sexp );
    while( l ) {
        Cons *u = asProperCons( l->getcar() );
        l = asCons( l->getcdr() );

        std::string username = *asString( u->nthcar(0) );
        std::string pwhash = *asString( u->nthcar(1) );
        std::string role = *asSymbol( u->nthcar(2) );

        users[ username ] = UserInfo( pwhash, role == "admin" );
    }
}

void Persistable::save(void) const {
    using namespace Sise;
    SExp *sexp = toSexp();
    if( !writeSExpToFile( filename, sexp ) ) {
        using namespace std;
        cerr << "warning: persistence failed" << endl;
    }
    delete sexp;
}

void Persistable::restore(void) {
    using namespace Sise;
    try {
        SExp *sexp = readSExpFromFile( filename );
        fromSexp( sexp );
        delete sexp;
    }
    catch( FileInputError& e ) {
    }
}

Persistable::Persistable(const std::string& filename) :
    filename ( filename )
{
}
UsersInfo::UserInfo& UsersInfo::operator[](const std::string& username) {
    UsersMap::iterator i = users.find( username );
    if( i == users.end() ) throw NoSuchUserException();
    return i->second;
}

const UsersInfo::UserInfo& UsersInfo::operator[](const std::string& username) const {
    UsersMap::const_iterator i = users.find( username );
    if( i == users.end() ) throw NoSuchUserException();
    return i->second;
}

bool UsersInfo::isAdministrator(const std::string& username) const {
    try {
        return (*this)[username].isAdmin;
    }
    catch( NoSuchUserException& e ) {
        return false;
    }
}

};
