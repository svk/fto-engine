#include "hexfml.h"

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>

#include <iostream>

#include <stdexcept>

int main(int argc, char *argv[]) {
    using namespace std;

    int sx = 0, sy = 0;

    ScreenGrid grid ( "./data/hexproto1.png" );

    HexSprite blue ( "./data/hexblue1.png", grid );

    sf::RenderWindow win ( sf::VideoMode(800,600,32),
                           "521 HexFML" );
    sf::Clock clock;

    sf::Image hexBorderImage;
    if( !hexBorderImage.LoadFromFile( "./data/hexborder1.png" ) ) {
        throw std::runtime_error( "unable to load hex border" );
    }
    sf::Sprite hexBorder;
    hexBorder.SetImage( hexBorderImage );

    int currentHexX = -1000,
        currentHexY = -1000;

    while( win.IsOpened() ) {
        sf::Event ev;
        const double dt = clock.GetElapsedTime();
        clock.Reset();

        while( win.GetEvent( ev ) ) switch( ev.Type ) {
            case sf::Event::Resized:
                win.SetView( sf::View( sf::Vector2f( 0, 0 ),
                                       sf::Vector2f( ev.Size.Width / 2, ev.Size.Height / 2 ) ) );
                break;
            case sf::Event::Closed:
                win.Close();
                break;
            case sf::Event::MouseMoved:
                {
                    int x = win.GetInput().GetMouseX(),
                        y = win.GetInput().GetMouseY();
                    if( x < 0 ) x = 0;
                    if( y < 0 ) y = 0;
                    sf::Vector2f p = win.ConvertCoords( x, y );
                    cerr << "p:" << p.x << " " << p.y << endl;
                    int hx = (int)(0.5+p.x), hy = (int)(0.5+p.y);
                    grid.screenToHex( hx, hy, sx, sy );
                    grid.hexToScreen( hx, hy );
                    currentHexX = hx;
                    currentHexY = hy;
                }
                break;
        }

        win.Clear( sf::Color(0,0,0) );

        blue.draw( win );

        hexBorder.SetPosition( currentHexX - sx + 0.5,
                               currentHexY - sy + 0.5 );
        win.Draw( hexBorder );

        win.Display();
    }

    return 0;
}
