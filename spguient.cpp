#include "sftools.h"
#include "typesetter.h"

#include <iostream>

#include <boost/program_options.hpp>
#include "SProto.h"

#include "NashClient.h"
#include "hexfml.h"

/* Doing interaction while weaseling out of writing GUI code: time
   to implement a CLI. We'll do the age-old method of having anything
   prepended by / be a command.

   What we need:
        Lobby functionality
            /users
            /challenge username
            /accept username
            /decline username
        Game functionality
            Pie rule:
                /swap
                [to not swap, just make a move]
            Ending the game:
                /resign
                /offer-draw
                /retract-draw
        Quitting
            /disconnect

    [note: parsing must be done client-side, so it's not awkward to
     plug in a GUI later]

    Note that none of the above commands have multiple arguments, so
    we can make it simple.
*/

class SpGuient : public SfmlApplication {
    private:
        SProto::Client& client;
        Sise::SocketManager manager;

    public:
        SpGuient(SProto::Client& client,
                 const std::string& username,
                 const std::string& password) :
            SfmlApplication( "SpGuient", 640, 480 ),
            client ( client )
        {
            using namespace Sise;

            client.debugSetOutputSpy( true );
            manager.adopt( &client );
            client.identify( username, password );
            while( !client.identified() ) {
                manager.pump(0,0);
                client.pump();
            }

            client.delsend( List()( new Symbol( "chat" ) )
                                  ( new Symbol( "join" ) )
                                  ( new String( "all" ) ).make() );
            client.delsend( List()( new Symbol( "nash" ) )
                                  ( new Symbol( "hello" ) )
                            .make() );
        }

        void tick(double dt) {
            manager.pump( 0, 0 );
            if( !manager.numberOfWatchedSockets() ) {
                stop();
            } else {
                client.pump();
            }
        }

        SProto::Client& getClient(void) { return client; }
};

class NashTPScreen : public SfmlScreen,
                     public SProto::ClientCore {
    private:
        FreetypeFace& font;
        SfmlApplication& app;
        SProto::Client& client;

        bool inputtingText;
        ChatBox chatbox;
        ChatInputLine chatinput;

        ScreenGrid& grid;
        Nash::NashBoard board;
        Nash::NashBlitter blitter;
        HexViewport viewport;

        bool expectingMove;
        bool maySwap;

        bool wasWelcomed;
        int gameId;
        std::string channelName;

        sf::Shape ipPanel, gamePanel;

        int width, height;

        void showServerMessage( const std::string& message ) {
            using namespace Sise;
            using namespace std;
            using namespace SProto;
            chatbox.add(ChatLine( "", sf::Color(255,255,255),
                                  message,sf::Color(100,100,200)));
        }

        void showMessage( Sise::Cons *msg ) {
            using namespace Sise;
            using namespace std;
            using namespace SProto;
            outputSExp( msg, cerr );
            if( getChatMessageOrigin(msg) == "" ) {
                showServerMessage( getChatMessageBody(msg) );
            } else {
                chatbox.add(ChatLine( getChatMessageOrigin(msg),sf::Color(128,128,128),
                                  getChatMessageBody(msg),sf::Color(255,255,255)));
            }
        }

        void handleNash( const std::string& cmd, Sise::SExp* arg) {
            using namespace std;
            using namespace Sise;
            if( cmd == "challenge" ) {
                std::ostringstream oss;
                std::string challenger = *asString( asProperCons(arg)->nthcar(0) );
                oss << ">> Received challenge from " << challenger;
                showServerMessage( oss.str() );
            } else if( cmd == "users" ) {
                std::ostringstream oss;
                oss << ">> Connected players: ";
                outputSExp( arg, cerr );
                Cons * t = asCons( arg );
                while( t ) {
                    oss << (std::string)*asString(t->getcar()) << " ";
                    t = asCons( t->getcdr() );
                }
                showServerMessage( oss.str() );
            } else if( cmd == "welcome" ) {
                wasWelcomed = true;
                gameId = *asInt( asProperCons(arg)->nthcar(0) );
                channelName = *asString( asProperCons(arg)->nthcar(1) );
                board.clear();
            } else if( !wasWelcomed ) {
                cerr << "oops: received " << cmd << " unexpectedly before welcome" << endl;
            } else if( *asInt(asProperCons(arg)->nthcar(0)) != gameId ) {
                cerr << "oops: received " << cmd << " to unknown gameId" << endl;
            } else if( cmd == "put" ) {
                std::string col = *asSymbol( asProperCons(arg)->nthcar(3));
                board.put( *asInt( asProperCons(arg)->nthcar(1)),
                           *asInt( asProperCons(arg)->nthcar(2)),
                           ("white" == col) ? Nash::NashTile::WHITE : Nash::NashTile::BLACK );
            } else if( cmd == "request-move" ) {
                std::string sym = *asSymbol( asProperCons(arg)->nthcar(1));
                expectingMove = true;
                maySwap = "may-swap" == sym;
            }
        }

    public:
        NashTPScreen( FreetypeFace& font,
                        SfmlApplication& app,
                        SProto::Client& client,
                        ScreenGrid& grid,
                        ResourceManager<HexSprite>& sprites) :
            font ( font ),
            app ( app ),
            client ( client ),
            inputtingText ( false ),
            chatbox ( 0, 0, 640, 480, font, sf::Color(0,0,0) ),
            chatinput ( 640, font, sf::Color(255,255,255), FormattedCharacter(font,sf::Color(255,255,0),'_') ),
            grid ( grid ),
            board ( 11 ),
            blitter ( board, sprites ),
            viewport ( grid, 0, 0, 640, 480 ),
            expectingMove ( false ),
            maySwap ( false ),
            wasWelcomed ( false )
        {
            client.setCore( this );
            viewport.setBackgroundColour( sf::Color(0,100,0) );
            
            showServerMessage( ">> Welcome to Hex!" );
            showServerMessage( ">> Commands: /users to get a list of connected users, /challenge <user> to challenge another user, /accept <challenger> to accept a challenge, /resign to resign a game." );
        }

        void resize(int width_, int height_) {
            const int totalBottomHeight = 200;
            const int ipWidth = 200;
            int cbHeight = totalBottomHeight;
            width = width_;
            height = height_;
            if( inputtingText ) {
                chatinput.setPosition( 0, height - cbHeight );
                cbHeight -= chatinput.getHeight();
            }
            chatbox.resize( width - ipWidth, cbHeight );
            chatinput.setWidth( width - ipWidth );
            chatbox.setPosition( 0, height - cbHeight );
            ipPanel = sf::Shape::Rectangle( width - ipWidth,
                                            0,
                                            width,
                                            height,
                                            sf::Color( 200, 128, 128 ) );
            viewport.setRectangle( 0,
                                   0,
                                   width - ipWidth,
                                   height - totalBottomHeight );
            viewport.center(0,0);
        }

        void toggleInputtingText(void) {
            inputtingText = !inputtingText;
            resize( width, height );
        }

        void handle( const std::string& name, Sise::SExp* sexp ) {
            using namespace Sise;
            Cons *args = asProperCons( sexp );
            if( name == "nash" ) {
                handleNash( *asSymbol( args->nthcar(0) ),
                            args->getcdr() );
                return;
            }
            if( name != "chat" ) return;
            using namespace std;
            outputSExp( args, cerr );
            std::string type = *asSymbol( args->nthcar(0) );
            if( type == "channel" ) {
                showMessage( asCons( args->nthtail( 3 ) ) );
            } else if( type == "private" || type == "broadcast" ) {
                showMessage( asCons( args->nthtail( 1 ) ) );
            }
        }

        void draw(sf::RenderWindow& win) {
            win.Clear( sf::Color(128,128,128) );
            chatbox.draw( win );
            if( inputtingText ) {
                chatinput.draw( win );
            }
            win.Draw( ipPanel );
            sf::View view ( sf::Vector2f(0,0),
                            sf::Vector2f((double)width/2.0, (double)height/2.0) );
            win.SetView( view );
            viewport.draw( blitter, win, view );
        }

        void handleCliCommand( const std::string& s ) {
            std::string command, argument;
            int b = -1;
            for(int i=0;i<(int)s.length();i++) {
                if( isspace( s[i] ) ) {
                    b = i;
                    break;
                }
            }
            if( b < 0 ) {
                command = s;
                argument = "";
            } else {
                command = s.substr( 0, b );
                argument = s.substr( b + 1 );
            }

            using namespace Sise;

            if( command == "challenge" ) {
                client.delsend( List()( new Symbol( "nash" ) )
                                      ( new Symbol( "challenge" ) )
                                      ( new String( argument ) )
                                .make() );
            } else if( command == "resign" ) {
                client.delsend( List()( new Symbol( "nash" ) )
                                      ( new Symbol( "resign" ) )
                                      ( new Int( gameId ) ).make() );
            } else if( command == "users" ) {
                client.delsend( List()( new Symbol( "nash" ) )
                                      ( new Symbol( "users" ) ).make() );
            } else if( command == "swap" ) {
                if( expectingMove && maySwap ) {
                    client.delsend( List()( new Symbol( "nash" ) )
                                          ( new Symbol( "swap" ) )
                                          ( new Int( gameId ) ).make() );
                    expectingMove = false;
                }
            } else if( command == "accept" ) {
                client.delsend( List()( new Symbol( "nash" ) )
                                      ( new Symbol( "accept" ) )
                                      ( new String( argument ) )
                                .make() );
            } else {
                showServerMessage( ":: bad command" );
            }
        }

        bool handleText(const sf::Event::TextEvent& text) {
            if( !inputtingText ) return false;
            chatinput.textEntered( text.Unicode );
            if( chatinput.isDone() ) {
                using namespace Sise;

                std::string data = chatinput.getString();
                chatinput.clear();
                toggleInputtingText();

                if( data.length() > 0 && data[0] == '/' ) {
                    handleCliCommand( data.substr( 1, data.length() - 1 ) );
                } else {
                    client.delsend( List()( new Symbol( "chat" ) )
                                          ( new Symbol( "channel-message" ) )
                                          ( new Symbol( "user" ) )
                                          ( new String( "all" ) )
                                          ( new String( data ) ).make() );
                }
            }
            return true;
        }

        bool handleLeftClick(int x, int y) {
            using namespace Sise;
            if( viewport.translateCoordinates( x, y ) ) {
                grid.screenToHex( x, y, 0, 0 );
                if( expectingMove && board.isLegalMove( x, y ) ) {
                    client.delsend( List()( new Symbol( "nash" ) )
                                          ( new Symbol( "move" ) )
                                          ( new Int( gameId ) )
                                          ( new Int( x ) )
                                          ( new Int( y ) ).make() );
                    expectingMove = false; // wait for new signal

                    if( board.getWinner() != Nash::NashTile::NONE ) {
                        chatbox.add(ChatLine( "", sf::Color(255,255,255),
                                              "Game over!",sf::Color(100,100,200)));
                    }
                }
            }
            return true;
        }
        
        bool handleMouseMoved(int x, int y) {
            bool doSelect = false;
            if( viewport.translateCoordinates( x, y ) ) {
                grid.screenToHex( x, y, 0, 0 );
                if( board.isLegalMove( x, y ) ) {
                    doSelect = true;
                }
            }
            if( expectingMove && doSelect ) {
                blitter.setSelected( x, y );
            } else {
                blitter.setNoSelected();
            }
            return true;
        }

        bool handleKey(const sf::Event::KeyEvent& key) {
            if( inputtingText ) {
                if( key.Code == sf::Key::Escape ) {
                    chatinput.clear();
                    toggleInputtingText();
                    return true;
                }
                return false;
            }
            switch( key.Code ) {
                case sf::Key::Q:
                    if( key.Shift ) {
                        app.stop();
                    }
                    break;
                case sf::Key::Return:
                    toggleInputtingText();
                    break;
                default: break;
            }
            return false;
        }

        bool handleEvent(const sf::Event& ev) {
            switch( ev.Type ) {
                case sf::Event::TextEntered:
                    return handleText( dynamic_cast<const sf::Event::TextEvent&>(ev.Text) );
                case sf::Event::KeyPressed:
                    return handleKey( dynamic_cast<const sf::Event::KeyEvent&>(ev.Key) );
                case sf::Event::MouseMoved:
                    return handleMouseMoved( app.getWindow().GetInput().GetMouseX(),
                                             app.getWindow().GetInput().GetMouseY() );
                case sf::Event::MouseButtonPressed:
                    switch( ev.MouseButton.Button ) {
                        case sf::Mouse::Left:
                            return handleLeftClick( app.getWindow().GetInput().GetMouseX(),
                                                    app.getWindow().GetInput().GetMouseY() );
                        default: break;
                    }
                    break;
                default: break;
            }
            return false;
        }
};

int main(int argc, char *argv[]) {
    FreetypeLibrary ftLib;
    FreetypeFace ftFont ("./data/CrimsonText-Bold.otf", 20);

    using namespace std;
    namespace po = boost::program_options;

    po::positional_options_description pd;

    pd.add( "host", 1 )
      .add( "port", 1 );

    po::options_description desc( "Allowed options" );
    desc.add_options()
        ("help", "display option help")
        ("noregister", "don't attempt to register an account upon login failure")
        ("username", po::value<string>(), "set username")
        ("password", po::value<string>(), "set password")
        ("host", po::value<string>(), "server host name or IP")
        ("port", po::value<int>()->default_value( SPROTO_STANDARD_PORT ), "server port")
        ;
    po::variables_map vm;
    po::store( po::command_line_parser( argc, argv ).options( desc ).positional( pd ).run(), vm );
    po::notify( vm );

    if( vm.count( "help" ) ||
        (!vm.count( "host") || !vm.count("username") || !vm.count("password")) ) {
        cout << "Usage: spguient <hostname> --username <username> --password <password>" << endl << endl;
        cout << desc << endl;
        return 1;
    }

    ScreenGrid grid ( "./data/hexproto2.png" );
    ResourceManager<HexSprite> sprites;
    sprites.bind( "tile-white", new HexSprite( "./data/hexwhite2.png", grid ) );
    sprites.bind( "tile-black", new HexSprite( "./data/hexblack2.png", grid ) );
    sprites.bind( "tile-free", new HexSprite( "./data/hexgray2grid.png", grid ) );
    sprites.bind( "border-selection", new HexSprite( "./data/hexborder2.png", grid ) );
    sprites.bind( "tile-edge-white", new HexSprite( "./data/hexwhite2.png", grid ) );
    sprites.bind( "tile-edge-black", new HexSprite( "./data/hexblack2.png", grid ) );
    sprites.bind( "tile-edge-white-black", new HexSprite( "./data/hexwhiteblack2.png", grid ) );
    sprites.bind( "tile-edge-black-white", new HexSprite( "./data/hexblackwhite2.png", grid ) );

    SProto::Client *client = Sise::connectToAs<SProto::Client>( vm["host"].as<string>(),
                                                                vm["port"].as<int>() );

    if( !vm.count( "noregister" ) ) { // at some point the default should probably be not autoregistering
                                      // (reason: typos in usernames)
                                      // this is fine for semi-long term testing though
        client->setAutoRegister();
    }

    SpGuient app ( *client,
                   vm["username"].as<string>(),
                   vm["password"].as<string>()
    );

    NashTPScreen mainScreen (ftFont, app, app.getClient(), grid, sprites );

    app.setScreen( &mainScreen );
    app.run();

    return 0;
}
