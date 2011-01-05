#include "hexfml.h"

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>

#include <cstdio>
#include <iostream>

#include <stdexcept>

ScreenGrid *gridp;
int selectedHexX, selectedHexY;
HexSprite *blue;
HexSprite *red;
HexSprite *ball;

class MyHexBlitter : public HexBlitter {
    public:
        void drawHex(int x, int y, sf::RenderWindow& win) {
            int ri, rj, rr;
            polariseHexCoordinate( x, y, ri, rj, rr );
            char buf[1024];
            sprintf( buf, "%d", flattenHexCoordinate(x,y) );
            sf::String text ( buf );
            sf::FloatRect rect = text.GetRect();
            gridp->centerRectangle( rect );

            if( (rr%2) == 0 ) {
                blue->draw( win );
            } else {
                red->draw( win );
            }
            if( x == selectedHexX && y == selectedHexY ) {
                ball->draw( win );
            }

            text.SetPosition( rect.Left, rect.Top );
            win.Draw( text );
        }
};

int main(int argc, char *argv[]) {
    using namespace std;

    int sx = 0, sy = 0;

    ScreenGrid grid ( "./data/hexproto1.png" );

    HexSprite bluev ( "./data/hexblue1.png", grid );
    HexSprite redv ( "./data/hexred1.png", grid );
    HexSprite ballv ( "./data/hexborder1.png", grid );
    gridp = &grid;
    blue = &bluev;
    red = &redv;
    ball = &ballv;

    MyHexBlitter blitter;

    bool showingKitten = false;

    HexViewport viewport (grid, 10,10,700,800);

    sf::RenderWindow win ( sf::VideoMode(800,600,32),
                           "521 HexFML" );
    sf::Clock clock;

    sf::Image hexBorderImage;
    if( !hexBorderImage.LoadFromFile( "./data/hexborder1.png" ) ) {
        throw std::runtime_error( "unable to load hex border" );
    }
    sf::Sprite hexBorder;
    hexBorderImage.SetSmooth( false );
    hexBorder.SetImage( hexBorderImage );

    sf::View mainView ( sf::Vector2f( 0, 0 ),
                        sf::Vector2f( 400, 300 ) );

    win.SetView( mainView );

    sf::Image kittenImage;
    if( !kittenImage.LoadFromFile( "./data/minikitten.png" ) ) {
        throw std::runtime_error( "unable to find kitten" );
    }
    sf::Sprite kittenSprite;
    kittenSprite.SetImage( kittenImage );
    kittenSprite.SetColor( sf::Color(255,255,255,225) );

    ViewportMouseScroller * vpMouseScroller = 0;

    while( win.IsOpened() ) {
        sf::Event ev;
        const double dt = clock.GetElapsedTime();
        clock.Reset();

        while( win.GetEvent( ev ) ) switch( ev.Type ) {
            case sf::Event::Resized:
                viewport.setRectangle( 10,
                                       10,
                                       (ev.Size.Width - 20),
                                       (ev.Size.Height - 20) );
                mainView.SetHalfSize( sf::Vector2f( ev.Size.Width / 2, ev.Size.Height / 2 ) );
                break;
            case sf::Event::Closed:
                win.Close();
                break;
            case sf::Event::MouseButtonReleased:
                if( ev.MouseButton.Button == sf::Mouse::Right ) {
                    if( vpMouseScroller ) {
                        delete vpMouseScroller;
                        vpMouseScroller = 0;
                        win.ShowMouseCursor(true);
                    }
                }
                break;
            case sf::Event::MouseButtonPressed:
                if( ev.MouseButton.Button == sf::Mouse::Right ) {
                    if( !vpMouseScroller ) {
                        win.ShowMouseCursor(false);
                        vpMouseScroller = new ViewportMouseScroller( viewport, win.GetInput() );
                    }
                }
                break;
            case sf::Event::MouseMoved:
                if( vpMouseScroller ) {
                    vpMouseScroller->scroll();
                } else {
                    int x = win.GetInput().GetMouseX(),
                        y = win.GetInput().GetMouseY();
                    int ix, iy;
                    ix = x;
                    iy = y;
                    if( viewport.translateCoordinates( ix, iy ) ) {
                        grid.screenToHex( ix, iy, 0, 0 );
                        selectedHexX = ix;
                        selectedHexY = iy;
                        if( flattenHexCoordinate( selectedHexX, selectedHexY ) == 42 ) {
                            sf::FloatRect rect = fitRectangleAt( x, y, sf::FloatRect( 0, 0, win.GetWidth(), win.GetHeight() ), kittenImage.GetWidth(), kittenImage.GetHeight() );
                            showingKitten = true;
                            cerr << "mouse finds kitten!" << rect.Left << " " << rect.Top <<  endl;
                            kittenSprite.SetPosition( rect.Left, rect.Top );
                        } else {
                            showingKitten = false;
                        }
                    }
                }
                break;
        }

        win.Clear( sf::Color(128,128,128) );

        viewport.draw( blitter, win, mainView );

        mainView.SetFromRect( sf::FloatRect( 0, 0, win.GetWidth(), win.GetHeight() ) );

        if( showingKitten ) {
            win.Draw( kittenSprite );
        }
        
        usleep( 10000 );

        win.Display();
    }

    return 0;
}
