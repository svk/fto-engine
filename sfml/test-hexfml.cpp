#include "hexfml.h"

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>

#include "typesetter.h"

#include <cstdio>
#include <iostream>

#include <stdexcept>

#include "anisprite.h"

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
    sf::Image kittenRednoseImage;
    if( !kittenRednoseImage.LoadFromFile( "./data/minikitten-clownnose.png" ) ) {
        throw std::runtime_error( "unable to find kitten-clown" );
    }
    sf::Sprite kittenSprite;
    kittenSprite.SetImage( kittenImage );
    kittenSprite.SetColor( sf::Color(255,255,255,225) );

    ViewportMouseScroller * vpMouseScroller = 0;

    FreetypeLibrary freetypeLib;
    FreetypeFace myFtFont ("./data/CrimsonText-Bold.otf", 20);

    sf::Image *textPopup = 0;
    {
        int popupWidth = 320;
        int spacing = 8;

        SfmlRectangularRenderer rr ( popupWidth, spacing, TJM_PAD );
        WordWrapper wrap ( rr, popupWidth, spacing );
        std::string texts[] = {
            "I have never had a way with ",
            "women",
            ", but the hills of ",
            "Iowa",
            " make me wish that I could. And I have never found a way to say ",
            "\"I love you\"",
            ", but if the chance came by, oh I, I ",
            "would",
            ". But way back where I come from, we never mean to bother, we " \
            "don't like to make our passions other people's concern, " \
            "and we walk in the world of safe people, " \
            "and at night we walk into our houses and burn."
        };
        sf::Color colours[] = {
            sf::Color(255,255,255),
            sf::Color(255,0,0),
            sf::Color(255,255,255),
            sf::Color(0,0,255),
            sf::Color(255,255,255),
            sf::Color(255,0,0),
            sf::Color(255,255,255),
            sf::Color(0,0,255),
            sf::Color(255,255,255)
        };
        for(int i=0;i<9;i++) {
            const char *s = texts[i].c_str();
            for(int j=0;j<strlen(s);j++) {
                wrap.feed( FormattedCharacter( myFtFont, colours[i], (uint32_t) s[j] ) );
            }
        }
        wrap.end();

        textPopup = rr.createImage();
    }

    AnimatedSprite kittenBlink ( 1.0, true );
    kittenBlink.addImage( kittenImage );
    kittenBlink.addImage( kittenRednoseImage );
    kittenBlink.SetColor( sf::Color(255,255,255,225) );

    sf::Sprite textSprite;
    textSprite.SetImage( *textPopup );

    sf::Shape textBackgroundBox;

    bool kittenMode = false;

    while( win.IsOpened() ) {
        sf::Event ev;
        const double dt = clock.GetElapsedTime();
        clock.Reset();

        kittenBlink.animate( dt );

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
                } else if( ev.MouseButton.Button == sf::Mouse::Left ) {
                    kittenMode = !kittenMode;
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
                            kittenBlink.SetPosition( rect.Left, rect.Top );
                            rect = fitRectangleAt( x, y, sf::FloatRect( 0, 0, win.GetWidth(), win.GetHeight() ), textPopup->GetWidth(), textPopup->GetHeight() );
                            textSprite.SetPosition( rect.Left, rect.Top );
                            textBackgroundBox = sf::Shape::Rectangle( rect.Left, rect.Top, rect.Right, rect.Bottom, sf::Color(64,64,64) );
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
            if( kittenMode ) {
                win.Draw( kittenBlink );
            } else {
                win.Draw( textBackgroundBox );
                win.Draw( textSprite );
            }
        }
        
        usleep( 10000 );

        win.Display();
    }

    delete textPopup;

    return 0;
}
