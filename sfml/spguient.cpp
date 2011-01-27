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

class NashTPScreen : public SfmlScreen {
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
        Nash::NashTile::Colour currentColour;

        sf::Shape ipPanel, gamePanel;

        int width, height;

        class ChatboxClientCore : public SProto::ClientCore {
            private:
                ChatBox& box;

            public:
                ChatboxClientCore(ChatBox& box) : box(box) {}
                
                void showMessage( Sise::Cons *msg ) {
                    using namespace Sise;
                    using namespace std;
                    using namespace SProto;
                    outputSExp( msg, cerr );
                    if( getChatMessageOrigin(msg) == "" ) {
                        box.add(ChatLine( "", sf::Color(255,255,255),
                                          getChatMessageBody(msg),sf::Color(100,100,200)));
                    } else {
                        box.add(ChatLine( getChatMessageOrigin(msg),sf::Color(128,128,128),
                                          getChatMessageBody(msg),sf::Color(255,255,255)));
                    }
                }

                void handle( const std::string& name, Sise::SExp* sexp ) {
                    using namespace Sise;
                    Cons *args = asProperCons( sexp );
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
        };

        ChatboxClientCore ccore;


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
            expectingMove ( true ), // xx: init to false and wait for signal
            currentColour ( Nash::NashTile::WHITE ),
            ccore ( chatbox )
        {
            client.setCore( &ccore );
            viewport.setBackgroundColour( sf::Color(0,100,0) );
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

        bool handleText(const sf::Event::TextEvent& text) {
            if( !inputtingText ) return false;
            chatinput.textEntered( text.Unicode );
            if( chatinput.isDone() ) {
                using namespace Sise;

                std::string data = chatinput.getString();
                chatinput.clear();
                toggleInputtingText();

                client.delsend( List()( new Symbol( "chat" ) )
                                      ( new Symbol( "channel-message" ) )
                                      ( new Symbol( "user" ) )
                                      ( new String( "all" ) )
                                      ( new String( data ) ).make() );
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
                                          ( new Int( x ) )
                                          ( new Int( y ) ).make() );
                    board.put( x, y, currentColour );
                    expectingMove = false; // wait for new signal

                    if( board.getWinner() != Nash::NashTile::NONE ) {
                        chatbox.add(ChatLine( "", sf::Color(255,255,255),
                                              "Game over!",sf::Color(100,100,200)));
                    } else {
                        expectingMove = true; // xx: not really yet
                        currentColour = ( currentColour == Nash::NashTile::WHITE ) ? Nash::NashTile::BLACK : Nash::NashTile::WHITE;
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
            if( doSelect ) {
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

    SpGuient app ( *Sise::connectToAs<SProto::Client>( vm["host"].as<string>(),
                                                      vm["port"].as<int>() ),
                   vm["username"].as<string>(),
                   vm["password"].as<string>()
    );

    NashTPScreen mainScreen (ftFont, app, app.getClient(), grid, sprites );

    app.setScreen( &mainScreen );
    app.run();

    return 0;
}
