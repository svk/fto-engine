#include "mtrand.h"

#include <cstdio>
#include <iostream>
#include <boost/program_options.hpp>

#include "typesetter.h"
#include "HexFov.h"
#include "TacClient.h"
#include "TacClientAction.h"

#include "sftools.h"
#include "SProto.h"

#include "TacRules.h"

using namespace Tac;

class TestTacTPScreen;


class TestTacTPScreen : public SfmlScreen,
                        public SProto::ClientCore {
    private:
        const int mapSize;
        FreetypeFace& font;
        ScreenGrid& grid;
        TacSpritesheet& sheet;
        ResourceManager<RandomVariantsCollection<sf::SoundBuffer> >& soundBuffers;
        ResourceManager<ClientTileType>& tileTypes;
        ResourceManager<ClientUnitType>& unitTypes;

        ClientMap cmap;
        int unitId;

        int width, height;
        HexViewport vp;

        SProto::Client& client;

        bool inputtingText;
        ChatBox chatbox;
        ChatInputLine chatinput;

    public:
        TestTacTPScreen(
            const int mapSize,
            FreetypeFace& font,
            ScreenGrid& grid,
            TacSpritesheet& sheet,
            ResourceManager<RandomVariantsCollection<sf::SoundBuffer> >& soundBuffers,
            ResourceManager<ClientTileType>& tileTypes,
            ResourceManager<ClientUnitType>& unitTypes,
            SProto::Client& client
        ) : mapSize (mapSize),
            font ( font ),
            grid ( grid ),
            sheet ( sheet ),
            soundBuffers ( soundBuffers ),
            tileTypes ( tileTypes ),
            unitTypes ( unitTypes ),
            cmap ( mapSize, sheet, grid, &font, tileTypes, unitTypes, soundBuffers ),
            unitId ( INVALID_ID ),
            width ( 640 ),
            height ( 480 ),
            vp ( grid,  0, 0, width, height ),
            client ( client ),
            inputtingText ( false ),
            chatbox ( 0, 0, width, height, font, sf::Color(0,50,0) ),
            chatinput ( width, font, sf::Color(255,255,255), FormattedCharacter(font,sf::Color(255,255,0),'_') )
        {
            resize( width, height );
        }

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

        void tick(double dt) {
            cmap.animate( dt );
            cmap.processActions();
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
/*
            ipPanel = sf::Shape::Rectangle( width - ipWidth,
                                            0,
                                            width,
                                            height,
                                            sf::Color( 200, 128, 128 ) );
*/
            vp.setRectangle( 0,
                                   0,
                                   width - ipWidth,
                                   height - totalBottomHeight );
        }

        void toggleInputtingText(void) {
            inputtingText = !inputtingText;
            resize( width, height );
        }

        void draw(sf::RenderWindow& win) {
            double cx, cy;
            ClientUnit* unit = 0;
            if( unitId != INVALID_ID ) {
                unit = cmap.getUnitById( unitId );
            }
            if( unit && cmap.getUnitBaseScreenPositionById( unitId, cx, cy ) ) {
                using namespace std;
                vp.center( cx, cy );
            }

            if( unit ) {
                ActivityPoints& acp = unit->getAP();
                struct CoreMove : public HexReceiver{
                    ClientMap& cmap;
                    int count;
                    CoreMove(struct ClientMap& cmap) : cmap(cmap), count(0) {}
                    void add(int x, int y){ ++count; cmap.addMoveHighlight( x, y ); };
                };
                struct OuterMove : public HexReceiver {
                    ClientMap& cmap;
                    int count;
                    OuterMove(struct ClientMap& cmap) : cmap(cmap), count(0) {}
                    void add(int x, int y) { ++count; cmap.addOuterMoveHighlight( x, y ); };
                };
                CoreMove coreMove( cmap );
                OuterMove outerMove( cmap );
                int x, y;
                cmap.clearHighlights();
                if( unit->getPosition( x, y ) ) {
                    using namespace std;
                    findAllAccessible( unit->getUnitType(), cmap, x, y, acp.getPotentialMovementEnergy(), outerMove );
                    findAllAccessible( unit->getUnitType(), cmap, x, y, acp.getImmediateMovementEnergy(), coreMove );
                    cmap.clearHighlight( x, y );
                    if( outerMove.count > 1 || unit->getAP().maySpendActionPoints(1) ) {
                        cmap.addMoveHighlight( x, y );
                    }
                }
            }

            win.Clear( sf::Color(0,50,0) );

            chatbox.draw( win );
            if( inputtingText ) {
                chatinput.draw( win );
            }

            glViewport( 0, 0, win.GetWidth(), win.GetHeight() );

            glMatrixMode( GL_PROJECTION );
            glLoadIdentity();

            glMatrixMode( GL_MODELVIEW );
            glLoadIdentity();
            sf::Matrix3 myMatrix;

            glEnable( GL_BLEND );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            sheet.bindTexture();

            glColor3f(1.0,1.0,1.0);

            vp.setBackgroundColour( sf::Color(0,0,0) );
            vp.drawGL( cmap.getLevelBlitter(), win, win.GetWidth(), win.GetHeight() );
            vp.setNoBackgroundColour();
            vp.drawGL( cmap.getUnitBlitter(0), win, win.GetWidth(), win.GetHeight() );


            vp.beginClip( win.GetWidth(), win.GetHeight() );
            win.SetView(sf::View( sf::Vector2f(0,0), sf::Vector2f( ((double)win.GetWidth())/2.0, ((double)win.GetHeight())/2.0 ) ) );
            using namespace std;

            sf::View fxView( sf::Vector2f(0,0), sf::Vector2f( ((double)win.GetWidth())/2.0, ((double)win.GetHeight())/2.0 ) );
            vp.translateToHex( 0, 0, win.GetWidth(), win.GetHeight(), fxView );
            win.SetView( fxView );
            cmap.drawEffects( win, 0, 0 );
            win.SetView( win.GetDefaultView() );

            vp.endClip();
        }

        bool handleKey(const sf::Event::KeyEvent& key) {
            using namespace Sise;
            bool doMove = true;
            int dx, dy;
            if( inputtingText ) {
                if( key.Code == sf::Key::Escape ) {
                    chatinput.clear();
                    toggleInputtingText();
                    return true;
                }
                return false;
            }
            switch( key.Code ) {
                case sf::Key::Return: toggleInputtingText(); return true;
                case sf::Key::Q: dx = -3; dy = 1; break;
                case sf::Key::W: dx = 0; dy = 2; break;
                case sf::Key::E: dx = 3; dy = 1; break;
                case sf::Key::A: dx = -3; dy = -1; break;
                case sf::Key::S: dx = 0; dy = -2; break;
                case sf::Key::D: dx = 3; dy = -1; break;
                default: doMove = false; break;
            }
            ClientUnit *me = cmap.getUnitById( unitId );
            if( doMove && unitId != INVALID_ID && me ) {
                int x, y;
                me->getPosition( x, y );
                x += dx;
                y += dy;
                int targetId = cmap.getTile( x, y ).getUnitIdAt( me->getLayer() );
                if( targetId != INVALID_ID ) {
                    client.delsend( List()( new Symbol( "tactest" ) )
                                          ( new Symbol( "melee-attack" ) )
                                          ( new Int( unitId ) )
                                          ( new Int( targetId ) )
                                    .make() );
                } else {
                    client.delsend( List()( new Symbol( "tactest" ) )
                                          ( new Symbol( "move-unit" ) )
                                          ( new Int( unitId ) )
                                          ( new Int( dx ) )
                                          ( new Int( dy ) )
                                    .make() );
                }
            }
            return false;
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
                    ;
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

        bool handleEvent(const sf::Event& ev) {
            switch( ev.Type ) {
                case sf::Event::KeyPressed:
                    return handleKey( ev.Key );
                case sf::Event::TextEntered:
                    return handleText( ev.Text );
                default: break;
            }
            return false;
        }

        void handleChat( Sise::Cons *chat ) {
            std::string type = *asSymbol( chat->nthcar(0) );
            if( type == "channel" ) {
                showMessage( asCons( chat->nthtail( 3 ) ) );
            } else if( type == "private" || type == "broadcast" ) {
                showMessage( asCons( chat->nthtail( 1 ) ) );
            }
        }

        void handle( const std::string& name, Sise::SExp* arg ) {
            using namespace Sise;
            if( name == "chat" ) {
                handleChat( asProperCons( arg ) );
            } else if( name == "tac" ) {
                Cons *args = asProperCons( arg );
                using namespace std;
                outputSExp( args, cerr );
                cmap.handleNetworkInfo( *asSymbol( args->getcar() ),
                                        args->getcdr() );
            } else if( name == "tactest" ) {
                Cons *args = asProperCons( arg );
                const std::string& subcmd = *asSymbol( args->nthcar(0) );
                if( subcmd == "welcome" ) {
                    using namespace std;
                    unitId = *asInt( args->nthcar(2) );
                }
            }
        }
};

class TestTacGuient : public SfmlApplication {
    private:
        SProto::Client& client;
        Sise::SocketManager manager;
        TestTacTPScreen *tttpsscreen;

    public:
        TestTacGuient(SProto::Client& client,
                 const std::string& username,
                 const std::string& password) :
            SfmlApplication( "TestTacGuient", 1024, 768 ),
            client ( client )
        {
            using namespace Sise;

            client.debugSetInputSpy( true );
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
            client.delsend( List()( new Symbol( "tactest" ) )
                                  ( new Symbol( "test-spawn" ) )
                            .make() );
        }

        void tick(double dt) {
            manager.pump( 0, 0 );
            if( !manager.numberOfWatchedSockets() ) {
                stop();
            } else {
                client.pump();
            }
            tttpsscreen->tick( dt );
        }

        void setTTScreen(TestTacTPScreen* tttpsscreen_) {
            tttpsscreen = tttpsscreen_;
            client.setCore( tttpsscreen );
            setScreen( tttpsscreen );
        }

        SProto::Client& getClient(void) { return client; }
};

int main(int argc, char *argv[]) {
    using namespace HexTools;
    using namespace Tac;
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
        cout << "Usage: test-tacclient <hostname> --username <username> --password <password>" << endl << endl;
        cout << desc << endl;
        return 1;
    }

    SProto::Client *client = Sise::connectToAs<SProto::Client>( vm["host"].as<string>(),
                                                                vm["port"].as<int>() );

    if( !vm.count( "noregister" ) ) { // at some point the default should probably be not autoregistering
                                      // (reason: typos in usernames)
                                      // this is fine for semi-long term testing though
        client->setAutoRegister();
    }

    TestTacGuient app ( *client,
                   vm["username"].as<string>(),
                   vm["password"].as<string>()
    );


    FreetypeLibrary lib;
    FreetypeFace risingTextFont ("./data/CrimsonText-Bold.otf", 20);

    const int mapSize = 10;

    ScreenGrid grid ( "./data/hexproto3.png" );

    TacSpritesheet sheet ( 1024, 1024 );
    loadSpritesFromFile( "./config/sprites.lisp", sheet, grid );

    ResourceManager<RandomVariantsCollection<sf::SoundBuffer> > soundBuffers;
    loadSoundsFromFile( "./config/sound-effects.lisp", soundBuffers );

    ResourceManager<ClientTileType> tileTypes;
    ResourceManager<ClientUnitType> unitTypes;

    unitTypes.bind( "pc", new ClientUnitType( "pc", sheet, "unit-smiley", "Player", 200, 84 ) );
    unitTypes.bind( "troll", new ClientUnitType( "troll", sheet, "unit-troll", "Troll", 500, 43 ) );

    tileTypes.bind( "border", new ClientTileType( "border", sheet, "tile-wall", "hard wall", Type::WALL, Type::BLOCK, true, 0 ) );
    tileTypes.bind( "floor", new ClientTileType( "floor", sheet, "tile-floor", "floor", Type::FLOOR, Type::CLEAR, false, 100 ) );
    tileTypes.bind( "wall", new ClientTileType( "wall", sheet, "tile-wall", "wall", Type::WALL, Type::BLOCK, false, 0 ) );

    TestTacTPScreen ttScreen ( mapSize, risingTextFont, grid, sheet, soundBuffers, tileTypes, unitTypes, *client );

    app.setTTScreen( &ttScreen );
    app.run();
    
    return 0;
}
