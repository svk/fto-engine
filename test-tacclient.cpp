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

        HexViewport vp;

    public:
        TestTacTPScreen(
            const int mapSize,
            FreetypeFace& font,
            ScreenGrid& grid,
            TacSpritesheet& sheet,
            ResourceManager<RandomVariantsCollection<sf::SoundBuffer> >& soundBuffers,
            ResourceManager<ClientTileType>& tileTypes,
            ResourceManager<ClientUnitType>& unitTypes
        ) : mapSize (mapSize),
            font ( font ),
            grid ( grid ),
            sheet ( sheet ),
            soundBuffers ( soundBuffers ),
            tileTypes ( tileTypes ),
            unitTypes ( unitTypes ),
            cmap ( mapSize, sheet, grid, &font, tileTypes, unitTypes ),
            unitId ( INVALID_ID ),
            vp ( grid,  0, 0, 640, 480 )
        {
        }

        void tick(double dt) {
            cmap.animate( dt );
            cmap.processActions();
        }

        void resize(int width, int height) {
            vp.setRectangle( 0, 0, width, height );
        }

        void draw(sf::RenderWindow& win) {
            double cx, cy;
            if( unitId != INVALID_ID && cmap.getUnitBaseScreenPositionById( unitId, cx, cy ) ) {
                vp.center( cx, cy );
            }

            win.Clear( sf::Color(0,0,0) );

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

            vp.drawGL( cmap.getLevelBlitter(), win, win.GetWidth(), win.GetHeight() );
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
            return false;
        }

        bool handleEvent(const sf::Event& ev) {
            switch( ev.Type ) {
                case sf::Event::KeyPressed:
                    return handleKey( ev.Key );
                default: break;
            }
            return false;
        }

        void handle( const std::string& name, Sise::SExp* arg ) {
            using namespace Sise;
            if( name == "tac" ) {
                Cons *args = asProperCons( arg );
                cmap.handleNetworkInfo( *asString( args->getcar() ),
                                        args->getcdr() );
            } else if( name == "tactest" ) {
                Cons *args = asProperCons( arg );
                const std::string& subcmd = *asString( args->nthcar(0) );
                if( subcmd == "welcome" ) {
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

    const int mapSize = 40;

    ScreenGrid grid ( "./data/hexproto2.png" );

    TacSpritesheet sheet ( 1024, 1024 );
    loadSpritesFromFile( "./config/sprites.lisp", sheet, grid );

    ResourceManager<RandomVariantsCollection<sf::SoundBuffer> > soundBuffers;
    loadSoundsFromFile( "./config/sound-effects.lisp", soundBuffers );

    ResourceManager<ClientTileType> tileTypes;
    ResourceManager<ClientUnitType> unitTypes;

    unitTypes.bind( "pc", new ClientUnitType( "pc", sheet, "unit-smiley", "Player" ) );
    unitTypes.bind( "troll", new ClientUnitType( "troll", sheet, "unit-troll", "Troll" ) );

    tileTypes.bind( "border", new ClientTileType( "border", sheet, "tile-wall", "hard wall", Type::WALL, Type::BLOCK, true, 0 ) );
    tileTypes.bind( "floor", new ClientTileType( "floor", sheet, "tile-floor", "floor", Type::FLOOR, Type::CLEAR, false, 100 ) );
    tileTypes.bind( "wall", new ClientTileType( "wall", sheet, "tile-wall", "wall", Type::WALL, Type::BLOCK, false, 0 ) );

    TestTacTPScreen ttScreen ( mapSize, risingTextFont, grid, sheet, soundBuffers, tileTypes, unitTypes );

    app.setTTScreen( &ttScreen );
    app.run();
    
    return 0;
}
