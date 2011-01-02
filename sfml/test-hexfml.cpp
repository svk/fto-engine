#include "hexfml.h"

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>

#include <iostream>

#include <stdexcept>

int main(int argc, char *argv[]) {
    using namespace std;

    ScreenGrid grid ( "./data/hexproto1.png" );

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
                win.SetView( sf::View( sf::FloatRect( 0,
                                                      0,
                                                      ev.Size.Width,
                                                      ev.Size.Height ) ) );
                break;
            case sf::Event::Closed:
                win.Close();
                break;
            case sf::Event::MouseMoved:
                {
                    int x = win.GetInput().GetMouseX(),
                        y = win.GetInput().GetMouseY();
                    grid.screenToHex( x, y, 0, 0 );
                    int hx = x, hy = y;
                    hy += 2; // HAX should be in the grid code
                    grid.hexToScreen( hx, hy );
                    cout << x << " " << y << ", ";
                    cout << hx << " " << hy << endl;
                    currentHexX = hx;
                    currentHexY = hy;
                }
                break;
        }

        win.Clear( sf::Color(0,0,0) );

        hexBorder.SetPosition( currentHexX,
                               currentHexY );
        win.Draw( hexBorder );

        win.Display();
    }

    return 0;
}
