#include "NashServer.h"

#include "SProto.h"
#include "Sise.h"

namespace Nash {

NashGame::NashGame(SProto::Server& server, int id, int size, const std::string white, const std::string black) :
    server ( server ),
    gameId ( id ),
    board ( size ),
    whitePlayer (white),
    blackPlayer (black),
    moveno ( 1 ),
    gameRunning ( true ),
    whiteToMove ( false ),
    swapAllowed ( false )
{
    using namespace SProto;
    using namespace Sise;

    std::ostringstream oss;
    oss << "game-" << gameId;
    channelName = oss.str();

    RemoteClient *wc = server.getConnectedUser( whitePlayer );
    RemoteClient *bc = server.getConnectedUser( blackPlayer );
    if( wc ) {
        wc->enterChannel( "nash", channelName );
    }
    if( bc ) {
        bc->enterChannel( "nash", channelName );
    }
    delbroadcast( List()( new Symbol( "nash" ) )
                        ( new Symbol( "welcome" ) )
                        ( new Int( gameId ) )
                        ( new String( channelName ) )
                  .make() );
    requestMove();
}

std::string NashGame::playerToMove(void) const {
    if( !gameRunning ) return "";
    if( whiteToMove ) return whitePlayer;
    return blackPlayer;
}

void NashGame::swap(const std::string& s) {
    if( swapAllowed && (s == playerToMove()) ) {
        using namespace std;
        std::ostringstream oss;
        oss << playerToMove() << " elected to swap colours, applying the pie rule. ";

        std::string t = blackPlayer;
        blackPlayer = whitePlayer;
        whitePlayer = t;

        swapAllowed = false;

        oss << whitePlayer << " is now playing white, and " << blackPlayer << " is playing black. ";
        
        broadcastMessage( oss.str() );

        broadcastInfo();
        requestMove();
    } else {
        respondIllegal( s );
    }
}

void NashGame::declareWin(NashTile::Colour colour) {
    gameRunning = false;
    std::ostringstream oss;
    oss << "Game over. ";
    oss << "Winner: " << ((colour == NashTile::WHITE) ? whitePlayer : blackPlayer);
    broadcastMessage( oss.str() );
}

void NashGame::move(const std::string& s, int x, int y) {
    using namespace std;
    NashTile::Colour c;
    bool didMove = false;
    if( gameRunning && whiteToMove && s == whitePlayer && board.isLegalMove( x, y ) ) {
        c = NashTile::WHITE;
        didMove = true;
    } else if( gameRunning && !whiteToMove && s == blackPlayer && board.isLegalMove(x,y) ) {
        c = NashTile::BLACK;
        didMove = true;
    } else {
        respondIllegal( s );
    }
    if( didMove ) {
        whiteToMove = !whiteToMove;
        broadcastMove( x, y, c );
        ++moveno;
        board.put( x, y, c );
        if( board.getWinner() == NashTile::NONE ) {
            broadcastInfo();
            swapAllowed = moveno == 2;
            requestMove();
        } else {
            declareWin( board.getWinner() );
        }
    }
}

void NashGame::requestMove(void) {
    std::ostringstream oss;
    oss << "Move " << moveno << ". ";
    oss << (whiteToMove ? "White" : "Black");
    oss << " (" << (whiteToMove ? whitePlayer : blackPlayer) << ")";
    oss << " to play.";
    if( swapAllowed ) {
        oss << " By the pie rule, " << playerToMove() << " may now choose to play " << (whiteToMove ? "black" : "white");
        oss << " instead of " << (whiteToMove ? "white" : "black") << ".";
        oss << " [type /swap to apply the pie rule]";
    }
    broadcastMessage( oss.str() );

    using namespace Sise;

    delsendTo( whiteToMove ? whitePlayer : blackPlayer,
               List()( new Symbol( "nash" ) )
                     ( new Symbol( "request-move" ) )
                     ( new Int( gameId ) )
                     ( new Symbol( swapAllowed ? "may-swap" : "may-not-swap" ) )
               .make() );
}

void NashGame::broadcastInfo(void) {
    // no metadata display yet, so no metadata to send
    // intention is stuff like move number, player to move, etc.
    // this is sent every turn

    // because of the pie rule, which player is which colour might change
    // during the game, so that's a natural fit as well
}

void NashGame::broadcastMove(int x, int y, NashTile::Colour c) {
    using namespace SProto;
    using namespace Sise;
    delbroadcast( List()( new Symbol( "nash" ) )
                        ( new Symbol( "put" ) )
                        ( new Int( gameId ) )
                        ( new Int( x ) )
                        ( new Int( y ) )
                        ( new Symbol( c == NashTile::WHITE ? "white" : "black" ) )
                  .make() );
}

void NashGame::respondIllegal(const std::string& username) {
    using namespace SProto;
    using namespace Sise;
    delsendTo( username,
               new Cons( new Symbol( "chat" ),
               new Cons( new Symbol( "channel" ),
               new Cons( new Symbol( "nash" ), 
               new Cons( new String( channelName ),
                         prepareChatMessage( "", "*** Illegal move." ))))));
}

void NashGame::broadcastMessage(const std::string& msg) {
    using namespace SProto;
    using namespace Sise;
    delbroadcast( new Cons( new Symbol( "chat" ),
                  new Cons( new Symbol( "channel" ),
                  new Cons( new Symbol( "nash" ), 
                  new Cons( new String( channelName ),
                            prepareChatMessage( "", "*** " + msg ))))));
}

void NashGame::broadcast(Sise::SExp* sexp) {
    sendTo( whitePlayer, sexp );
    sendTo( blackPlayer, sexp );
}

void NashGame::sendTo(const std::string& username, Sise::SExp* sexp) {
    SProto::RemoteClient *rc = server.getConnectedUser( username );
    if( rc ) {
        rc->send( sexp );
    }
}

void NashGame::delbroadcast(Sise::SExp* sexp) {
    broadcast( sexp );
    delete sexp;
}


void NashGame::delsendTo(const std::string& username, Sise::SExp* sexp) {
    sendTo( username, sexp );
    delete sexp;
}

bool NashGame::handle(const std::string& username, const std::string& cmd, Sise::SExp* arg) {
    using namespace Sise;
    if( cmd == "move" ) {
        using namespace std;
        int x = *asInt(asProperCons(arg)->nthcar(0));
        int y = *asInt(asProperCons(arg)->nthcar(1));
        move( username, x, y );
        return true;
    } else if( cmd == "swap" ) {
        swap( username );
        return true;
    } else if( cmd == "resign" ) {
        if( !gameRunning ) return false;
        std::ostringstream oss;
        if( username == whitePlayer ) {
            oss << "White (" << username << ") resigns.";
            broadcastMessage( oss.str() );
            declareWin( NashTile::BLACK );
        } else if( username == blackPlayer ) {
            oss << "Black (" << username << ") resigns.";
            broadcastMessage( oss.str() );
            declareWin( NashTile::WHITE );
        } else {
            return false;
            // wot?
        }
        return true;
    } else return false;
}

bool NashGame::done(void) const {
    return false; // xx: games just live forever
}

NashSubserver::NashSubserver( SProto::Server& server ) :
    Persistable( "./persist/nash.lisp" ),
    SubServer( "nash", server ),
    nextGameId ( 0 ),
    games (),
    challenges ()
{
}

void NashSubserver::fromSexp(Sise::SExp *sexp) {
    using namespace Sise;
    nextGameId = *asInt( asProperCons(sexp)->nthcar(0) );
}

Sise::SExp * NashSubserver::toSexp(void) const {
    using namespace Sise;
    // this is more useful when we implement persisting
    // actual games in progress (adjourning)
    // a contained directory persistable might be useful for that
    // (one file per game in progress)
    return List()( new Int( nextGameId ) )
           .make();
}

bool NashSubserver::handle( SProto::RemoteClient* cli, const std::string& cmd, Sise::SExp* arg) {
    using namespace Sise;
    using namespace SProto;
    if( cmd == "move" || cmd == "swap" || cmd == "resign" ) { // xx inelegant
        GameMap::iterator i = games.find( *asInt(asProperCons(arg)->nthcar(0)) );
        if( i == games.end() ) {
            return false;
        }
        return i->second->handle( cli->getUsername(), cmd, asProperCons(arg)->getcdr() );
    }
    if( cmd == "hello" ) {
        players.push_back( cli->getUsername() );
    } else if( cmd == "users" ) {
        pruneUsers();
        List list;
        list( new Symbol( "nash" ) )
            ( new Symbol( "users" ) );
        for(PlayerList::iterator i = players.begin(); i != players.end(); i++) {
            list( new String(*i) );
        }
        cli->delsend( list.make() );
    } else if( cmd == "challenge" ) {
        std::string challengee = *asString( asProperCons(arg)->nthcar(0) );

        if( cli->getUsername() == challengee ) {
            return false;
        }

        // check?
        challenges[ cli->getUsername() ] = challengee;

        RemoteClient *rc = server.getConnectedUser( challengee );
        if( rc ) {
            rc->delsend( List()( new Symbol( "nash" ) )
                               ( new Symbol( "challenge" ) )
                               ( new String( cli->getUsername() ) )
                         .make() );
        }
    } else if( cmd == "accept" ) {
        std::string challenger = *asString( asProperCons(arg)->nthcar(0) );

        // check that there is such a challenge..
        std::map<std::string, std::string>::iterator i = challenges.find( challenger );
        if( i != challenges.end() && i->second == cli->getUsername() ) {
            // for now the challenger/challengee status determines colour
            // the pie rule _does_ make this perfectly fair in any case, so doesn't matter much
            int id = nextGameId++;
            save(); // as far as possible we want to keep game numbers unique
                    // might want to save only sequence # there for scalability
            NashGame *game = new NashGame( server, id, 11, challenger, cli->getUsername() );
            games[ id ] = game;
        }
    } else {
        return false;
    }
    return true;
}

NashSubserver::~NashSubserver(void) {
    for(GameMap::iterator i = games.begin(); i != games.end(); i++) {
        delete i->second;
    }
    games.clear();
}

}
