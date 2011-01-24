#include "SProto.h"

#define PROTOCOL_ID "SProto"
#define PROTOCOL_VERSION 1
#define PROTOCOL_SERVER_ID "StdServer"
#define PROTOCOL_CLIENT_ID "StdClient"

#include <ctime>

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
    rclients ( rclients ),
    loggingIn ( false )
{
    rclients.push_back( this );
}

void RemoteClient::setState(RemoteClient::State state_) {
    state = state_;
}

RemoteClient::State RemoteClient::getState(void) const {
    return state;
}

std::string RemoteClient::getUsername(void) const {
    return username;
}

bool RemoteClient::hasUsername(void) const {
    return username != "";
}

RemoteClient::~RemoteClient(void) {
    using namespace std;
    rclients.erase( remove( rclients.begin(), rclients.end(), this ) );
}

void SProtoSocket::close(void) {
    closing = true;
    gracefulShutdown();
}

void RemoteClient::handle( const std::string& cmd, Sise::SExp *arg ) {
    server.handle( this, cmd, arg );
}

bool AdminSubserver::handle( RemoteClient *cli, const std::string& cmd, Sise::SExp *arg ) {
    using namespace Sise;
    if( !cli->hasUsername() || !server.getUsers().isAdministrator( cli->getUsername() ) ) {
        cli->delsendResponse( cmd, "permission denied" );
        return true;
    }
    if( cmd == "shutdown" ) {
        server.stopServer();
    } else if( cmd == "save" ) {
        server.save();
    } else if( cmd == "change-any-password" ) {
        Cons *args = asProperCons( arg );
        std::string username = *asString( args->nthcar(0) );
        std::string password = *asString( args->nthcar(1) );
        try {
            server.getUsers()[username].passwordhash = makePasswordHash( username, password );
            cli->delsendResponse( "change-any-password", "ok" );
        }
        catch( NoSuchUserException& e ) {
            cli->delsendResponse( "change-any-password", "no such user" );
        }
    } else {
        return false;
    }
    return true;
}

bool UsersInfo::handle( RemoteClient *cli, const std::string& cmd, Sise::SExp *arg ) {
    using namespace Sise;
    if( cmd == "login-request" ) {
        Cons *args = asProperCons( arg );
        std::string challenge = server.makeChallenge();
        cli->setLoggingIn( *asString( args->nthcar(0) ),
                           challenge );
        cli->delsendPacket( "login-challenge",
                       List()( new String( *asString( args->nthcar(0) ) ) )
                             ( new String( challenge ) )
                       .make() );
    } else if( cmd == "check-username" ) {
        Cons *args = asProperCons( arg );
        std::string uname = *asString( args->nthcar(0) );
        cli->delsendPacket( "check-username-response",
                       List()( new String(uname) )
                             ( new String( usernameAvailable(uname) ) )
                       .make() );
    } else if( cmd == "register" ) {
        Cons *args = asProperCons( arg );
        std::string uname = *asString( args->nthcar(0) );
        std::string pword = *asString( args->nthcar(1) );
        std::string failureReason = registerUsername( uname, pword );
        if( failureReason != "ok" ) {
            cli->delsendPacket( "register-failure",
                           List()( new String( failureReason ))
                           .make() );
        } else {
            cli->delsendPacket( "register-ok",
                           List()( new String( uname ) )
                           .make() );
        }

    } else if( cmd == "login-response" ) {
        Cons *args = asProperCons( arg );
        std::string desiredUsername, challenge;
        if( !cli->getLoggingIn( desiredUsername, challenge ) ) {
            cli->close();
        } else {
            cli->setNotLoggingIn();
            std::string response = *asString( args->nthcar(0) );
            try {
                if( response == solveChallenge( desiredUsername,
                                                       challenge ) ) {
                    cli->setUsername( desiredUsername );
                    cli->delsendPacket( "login-ok",
                                   List()( new String( desiredUsername ) )
                                   .make() );
                } else {
                    cli->delsendPacket( "login-failure",
                                   List()( new String( "login failed" ))
                                   .make() );
                }
            }
            catch( NoSuchUserException& e ) {
                cli->delsendPacket( "login-failure",
                               List()( new String( "login failed" ))
                               .make() );
            }
        }
    } else if( cmd == "change-password" ) {
        if( !cli->hasUsername() ) {
            cli->delsendResponse( cmd, "permission denied" );
        } else {
            Cons *args = asProperCons( arg );
            std::string username = cli->getUsername();
            std::string password = *asString( args->nthcar(0) );
            users[username].passwordhash = makePasswordHash( username, password );
            cli->delsendPacket( "change-password-ok", 0 );
        }
    } else {
        return false;
    }
    return true;
}

bool DebugSubserver::handle( RemoteClient *cli, const std::string& cmd, Sise::SExp *arg ) {
    using namespace Sise;
    if( cmd == "hash" ) {
        Cons *args = asProperCons( arg );
        std::string data = *asString( args->nthcar(0) );
        cli->delsendPacket( "debug-reply",
                       List()( new String( getHash( data ) ) )
                       .make() );
    } else if( cmd == "who-am-i" ) {
        cli->delsendPacket( "debug-reply",
                       List()( new String( cli->getUsername() ) )
                             ( new String( cli->getNetId() ) )
                       .make() );
    } else if( cmd == "password-hash" ) {
        // this is here for convenience in case the admin password is forgotten..
        std::string username = *asString( asProperCons(arg)->nthcar(0) );
        std::string password = *asString( asProperCons(arg)->nthcar(1) );
        cli->delsend( List()( new Symbol( "response" ) )
                            ( new Symbol( "password-hash" ) )
                            ( new String( makePasswordHash( username, password ) ) )
                      .make() );
    } else {
        return false;
    }
    return true;
}

void Server::handle( RemoteClient *cli, const std::string& cmd, Sise::SExp *arg ) {
    using namespace Sise;
    if( cmd == "hello" ) {
        Cons *args = asProperCons( arg );
        std::string protoName = *asSymbol( args->nthcar(0) );
        int majorVersion = *asInt( args->nthcar(1) );
        std::string clientName = *asString( args->nthcar(2) );
        if( cli->getState() != RemoteClient::ST_SILENT ||
            protoName != PROTOCOL_ID ||
            majorVersion != PROTOCOL_VERSION ) {
            cli->close();
        } else {
            cli->setState( RemoteClient::ST_VERSION_OK );
            cli->delsendPacket( "hello",
                List()( new String( PROTOCOL_SERVER_ID ) )
                .make() );
        }
    } else if( cli->getState() == RemoteClient::ST_SILENT ) {
        cli->close();
    } else if( getSubServer( cmd ) ) { 
        SubServer *subserv = getSubServer( cmd );
        Cons *args = asProperCons( arg );
        Symbol *cmdp = asSymbol( args->getcar() );
        SExp *argp = args->getcdr();
        if( !subserv->handle( cli, *cmdp, argp ) ) {
            cli->delsendResponse( cmd, "bad-command" );
        }
        return;
    } else if( cmd == "goodbye" ) {
        cli->close();
    } else {
        cli->delsendResponse( cmd, "bad-command" );
    }
}

void SProtoSocket::delsendResponse( const std::string& name, const std::string& content ) {
    using namespace Sise;
    delsend( List()( new Symbol( "response" ) )
                   ( new Symbol( name ) )
                   ( new String( content ) )
             .make() );
}

void SProtoSocket::delsendPacket( const std::string& name, Sise::SExp* cdr ) {
    using namespace Sise;
    delsend( new Cons( new Symbol( name ), cdr ) );
}

void SProtoSocket::send( Sise::SExp* sexp ) {
    if( !closing ) {
        Sise::outputSExp( sexp, out() );
    }
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

SubServer::SubServer(const std::string& name, Server& server) :
    server ( server )
{
    server.setSubServer( name, this );
}

Server::Server(void) :
    ConsSocketManager (),
    rclients (),
    subservers (),
    running ( true ),
    users ( *this ),
    ssDebug ( *this ),
    ssAdmin ( *this )
{
    setGreeter( this );

    restore();
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

void Server::restore(void) {
    for(SubserverMap::iterator i = subservers.begin(); i != subservers.end(); i++) {
        i->second->restoreSubserver();
    }
}

void Server::save(void) {
    for(SubserverMap::iterator i = subservers.begin(); i != subservers.end(); i++) {
        i->second->saveSubserver();
    }
}

Server::~Server(void) {
    unwatchAll();

    save();
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

        delsend( List()( new Symbol( "user" ) )
                       ( new Symbol( "login-response" ) )
                       ( new String( response ) )
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
    delsend( List()( new Symbol( "user" ) )
                   ( new Symbol( "login-request" ) )
                   ( new String( username ) )
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

DirectoryPersistable::DirectoryPersistable(const std::string& dirname) :
    dirname ( dirname ),
    extension ( ".lisp" )
{
}

void DirectoryPersistable::writeNamedSexp(const std::string& name, Sise::SExp* sexp) {
    if( !Sise::writeSExpToFile( dirname + "/" + name, sexp ) ) {
        using namespace std;
        cerr << "warning: writing to named sexp " << name << " in " << dirname << " failed" << endl;
    }
}

void DirectoryPersistable::clearFiles(void) {
    Sise::removeAllFilesWithExtension( dirname, extension );
}

void DirectoryPersistable::restore(void) {
    readSExpDir( dirname, extension, *this );
}

Sise::SExp *prepareChatMessage(const std::string& origin, const std::string& message) {
    using namespace Sise;
    return new Cons( new String( origin ),
           new Cons( new String( message ),
           new Cons( new Int ( time(0) ))));
}

bool ChatSubserver::handle( RemoteClient *cli, const std::string& cmd, Sise::SExp* arg ) {
    using namespace Sise;

    Server::RClientList::iterator i = server.getClients().begin();
    Server::RClientList::iterator end = server.getClients().end();

    if( !cli->hasUsername() ) {
        cli->delsendResponse( cmd, "cannot use chat without being logged in" );
        return true;
    }

    if( cmd == "channel-message" || cmd == "cm" ) {
        std::string channelType = *asSymbol( asProperCons(arg)->nthcar(0) );
        std::string channelName = *asString( asProperCons(arg)->nthcar(1) );
        if( !cli->isInChannel( channelType, channelName ) ) {
            cli->delsendResponse( cmd, "not in that channel" );
        } else {
            SExp * sexp = new Cons( new Symbol( "chat" ),
                          new Cons( new Symbol( "channel" ),
                          new Cons( new Symbol( channelType ),
                          new Cons( new String( channelName ),
                                    prepareChatMessage( cli->getUsername(),
                                                        *asString( asProperCons(arg)->nthcar(0)))))));
            while( i != end ) {
                if( (*i)->isInChannel( channelType, channelName ) ) {
                    (*i)->send( sexp );
                }
                i++;
            }
            delete sexp;
        }
    } else if( cmd == "private-message" || cmd == "pm" ) {
        std::string targetName = *asString( asProperCons(arg)->nthcar(1) );
        RemoteClient *target = 0;
        int hits = 0;
        while( i != end ) {
            if( (*i)->hasUsername() && (*i)->getUsername() == targetName ) {
                hits++;
                target = *i;
            }
        }
        assert( hits < 2 );
        if( target ) {
            SExp * sexp = new Cons( new Symbol( "chat" ),
                          new Cons( new Symbol( "private" ),
                                    prepareChatMessage( cli->getUsername(),
                                                        *asString( asProperCons(arg)->nthcar(0)))));
            target->send( sexp );
            delete sexp;
        } else {
            cli->delsendResponse( cmd, "no such user" );
        }
    } else if( cmd == "join" ) {
        cli->enterChannel( "user", *asString( asProperCons( arg )->nthcar(0) ) );
    } else if( cmd == "part" ) {
        cli->leaveChannel( "user", *asString( asProperCons( arg )->nthcar(0) ) );
    } else if( cmd == "broadcast" || cmd == "bc" ) {
        if( !server.getUsers().isAdministrator( cli->getUsername() ) ) {
            cli->delsendResponse( cmd, "permission denied" );
        } else {
            SExp * sexp = new Cons( new Symbol( "chat" ),
                          new Cons( new Symbol( "broadcast" ),
                                    prepareChatMessage( cli->getUsername(),
                                                        *asString( asProperCons(arg)->nthcar(0) ) ) ) );
            while( i != end ) {
                (*i)->send( sexp );
                i++;
            }
            delete sexp;
        }
    } else {
        return false;
    }
    return true;
}

};
