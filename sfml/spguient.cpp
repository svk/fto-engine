#include "sftools.h"
#include "typesetter.h"

#include <iostream>

#include <boost/program_options.hpp>
#include "SProto.h"

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

class TriPanelScreen : public SfmlScreen {
    private:
        FreetypeFace& font;
        SfmlApplication& app;
        SProto::Client& client;

        bool inputtingText;
        ChatBox chatbox;
        ChatInputLine chatinput;

        sf::Shape ipPanel, gamePanel;

        int width, height;

    public:
        TriPanelScreen( FreetypeFace& font,
                        SfmlApplication& app,
                        SProto::Client& client) :
            font ( font ),
            app ( app ),
            client ( client ),
            inputtingText ( false ),
            chatbox ( 0, 0, 640, 480, font, sf::Color(0,0,0) ),
            chatinput ( 640, font, sf::Color(255,255,255), FormattedCharacter(font,sf::Color(255,255,0),'_') )
        {
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
            gamePanel = sf::Shape::Rectangle( 0,
                                              0,
                                              width - ipWidth,
                                              height - totalBottomHeight,
                                              sf::Color( 200, 200, 128 ) );
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
            win.Draw( gamePanel );
        }

        bool handleText(const sf::Event::TextEvent& text) {
            if( !inputtingText ) return false;
            chatinput.textEntered( text.Unicode );
            if( chatinput.isDone() ) {
                using namespace Sise;

                std::string data = chatinput.getString();
                chatinput.clear();
                toggleInputtingText();
                chatbox.add(ChatLine("kaw",sf::Color(128,128,128),
                                     data,sf::Color(255,255,255)));

                client.delsend( List()( new Symbol( "chat" ) )
                                      ( new Symbol( "channel-message" ) )
                                      ( new Symbol( "user" ) )
                                      ( new String( "all" ) )
                                      ( new String( data ) ).make() );
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

    if( vm.count( "help" ) ) {
        cout << desc << endl;
        return 1;
    }

    SpGuient app ( *Sise::connectToAs<SProto::Client>( vm["host"].as<string>(),
                                                      vm["port"].as<int>() ),
                   vm["username"].as<string>(),
                   vm["password"].as<string>()
    );

    TriPanelScreen mainScreen (ftFont, app, app.getClient());

    app.setScreen( &mainScreen );
    app.run();

    return 0;
}
