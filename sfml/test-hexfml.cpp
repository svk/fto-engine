#include "hexfml.h"

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "typesetter.h"

#include <cstdio>
#include <iostream>

#include <stdexcept>

#include "anisprite.h"

#include "sftools.h"

#include "HtGo.h"

struct MyTile {
    enum MyColour {
        NEUTRAL,
        WHITE,
        BLACK,
        WHITE_EDGE,
        BLACK_EDGE,
        WHITEBLACK_EDGE,
        BLACKWHITE_EDGE,
        OFF_MAP
    };

    MyColour tileType;
    
    MyTile(void) :
        tileType( OFF_MAP )
    {
    }
};

class MyHexBlitter : public HexBlitter {
    private:
        const ScreenGrid& grid;
        ResourceManager<HexSprite>& hexSprites;
        HexMap<MyTile>& myMap;
        HexTorusGoMap& tgMap;

        int selectedHexX, selectedHexY;
        bool showNumbers;
        bool tgMode;

    public:
        MyHexBlitter( const ScreenGrid& grid, ResourceManager<HexSprite>& hexSprites, HexMap<MyTile>& myMap, HexTorusGoMap& tgMap ) :
            grid ( grid ),
            hexSprites ( hexSprites ),
            myMap (myMap ),
            tgMap (tgMap ),
            selectedHexX (-1000),
            selectedHexY (-1000),
            showNumbers (false),
            tgMode (false)
        {
        }

        void toggleTGMode(void) {
            tgMode = !tgMode;
        }

        void toggleShowNumbers(void) {
            showNumbers = !showNumbers;
        }

        void setSelected(int x, int y) {
            if( !tgMode ) {
                selectedHexX = x;
                selectedHexY = y;
            } else {
                HtGoTile& tile = tgMap.get( x, y );
                selectedHexX = tile.coreX;
                selectedHexY = tile.coreY;
            }
        }

        void drawHexTG(int x, int y, sf::RenderWindow& win) {
            HtGoTile& tile = tgMap.get(x,y);
            sf::String text ( tile.positionalLabel );
            sf::FloatRect rect = text.GetRect();
            grid.centerRectangle( rect );
            text.SetPosition( rect.Left, rect.Top );
            text.SetColor( sf::Color( 200, 0, 0 ) );
//            win.Draw( text );
            switch( tile.state ) {
                case HtGoTile::BLACK:
                    hexSprites["black"].draw( win );
                    break;
                case HtGoTile::WHITE:
                    hexSprites["white"].draw( win );
                    break;
                case HtGoTile::BLANK:
                    hexSprites["gray"].draw( win );
                    break;
            }
            if( tile.coreX == selectedHexX && tile.coreY == selectedHexY ) {
                hexSprites["yellow-border"].draw( win );
            }
        }

        bool getTGMode(void) {
            return tgMode;
        }

        void drawHex(int x, int y, sf::RenderWindow& win) {
            if( tgMode ) {
                drawHexTG(x,y,win);
                return;
            }
            int ri, rj, rr;
            polariseHexCoordinate( x, y, ri, rj, rr );
            char buf[1024];
            sprintf( buf, "%d", flattenHexCoordinate(x,y) );
            sf::String text ( buf );
            sf::FloatRect rect = text.GetRect();
            grid.centerRectangle( rect );

            switch( myMap.get(x,y).tileType ) {
                case MyTile::WHITEBLACK_EDGE:
                    hexSprites["white-black-edge"].draw( win );
                    break;
                case MyTile::BLACKWHITE_EDGE:
                    hexSprites["black-white-edge"].draw( win );
                    break;
                case MyTile::WHITE:
                case MyTile::WHITE_EDGE:
                    hexSprites["white"].draw( win );
                    break;
                case MyTile::NEUTRAL:
                    hexSprites["gray"].draw( win );
                    break;
                case MyTile::BLACK:
                case MyTile::BLACK_EDGE:
                    hexSprites["black"].draw( win );
                    break;
                case MyTile::OFF_MAP:
                    return;
            }

            if( x == selectedHexX && y == selectedHexY ) {
                hexSprites["yellow-border"].draw( win );
            }

            if( showNumbers && myMap.get(x,y).tileType == MyTile::NEUTRAL ) {
                text.SetPosition( rect.Left, rect.Top );
                text.SetColor( sf::Color( 200, 0, 0 ) );
                win.Draw( text );
            }
        }
};

int main(int argc, char *argv[]) {
    using namespace std;

    ResourceManager<sf::Image> images;
    ResourceManager<HexSprite> hexSprites;
    ResourceManager<sf::SoundBuffer> soundBuffers;

    ScreenGrid grid ( "./data/hexproto1.png" );
    hexSprites.bind( "black", new HexSprite( "./data/hexblack1.png", grid ) );
    hexSprites.bind( "white", new HexSprite( "./data/hexwhite1.png", grid ) );
    hexSprites.bind( "gray", new HexSprite( "./data/hexgray1.png", grid ) );
    hexSprites.bind( "red", new HexSprite( "./data/hexred1.png", grid ) );
    hexSprites.bind( "blue", new HexSprite( "./data/hexblue1.png", grid ) );
    hexSprites.bind( "rainbow", new HexSprite( "./data/hexrainbow1.png", grid ) );
    hexSprites.bind( "yellow-border", new HexSprite( "./data/hexborder1.png", grid ) );
    hexSprites.bind( "white-black-edge", new HexSprite( "./data/hexwhiteblack1.png", grid ) );
    hexSprites.bind( "black-white-edge", new HexSprite( "./data/hexblackwhite1.png", grid ) );

    soundBuffers.bind( "fail-sound", new sf::SoundBuffer() );
    soundBuffers["fail-sound"].LoadFromFile( "./data/fail.wav" );

    bool showingKitten = false;

    HexViewport viewport (grid, 10,10,700,800);
    viewport.setNoBackgroundColour();

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
            for(int j=0;j<(int)strlen(s);j++) {
                wrap.feed( FormattedCharacter( myFtFont, colours[i], (uint32_t) s[j] ) );
            }
        }
        wrap.end();

        textPopup = rr.createImage();
    }

    // The radial HexMap shape is actually incredibly unsuitable
    // for this map shape. Radial was of course written with other
    // applications in mind (e.g. a scrollable game map where
    // rectangularity is just awkward).
    HexMap<MyTile> myMap( 11 );
    myMap.getDefault().tileType = MyTile::OFF_MAP;
    for(int i=-6;i<=6;i++) for(int j=-6;j<=6;j++) {
        int x = i * 3, y = 2 * j + i;
        MyTile::MyColour type = MyTile::NEUTRAL;
        if( abs(i) == 6 && abs(j) == 6 ) {
            continue;
        }
        if( abs(i) == 6 ) type = MyTile::WHITE_EDGE;
        if( abs(j) == 6 ) type = MyTile::BLACK_EDGE;
        myMap.get(x,y).tileType = type;
    }
    myMap.get(-18,6).tileType = MyTile::BLACKWHITE_EDGE;
    myMap.get(18,-6).tileType = MyTile::WHITEBLACK_EDGE;

    AnimatedSprite kittenBlink ( 1.0, true );
    kittenBlink.addImage( kittenImage );
    kittenBlink.addImage( kittenRednoseImage );
    kittenBlink.SetColor( sf::Color(255,255,255,225) );

    sf::Sprite textSprite;
    textSprite.SetImage( *textPopup );

    sf::Shape textBackgroundBox;

    bool kittenMode = false;

    HexTorusGoMap myTorusMap(5);
    myTorusMap.debugLabelCore();

    MyHexBlitter blitter (grid, hexSprites, myMap, myTorusMap);

    HtGoTile::TileState myCurrentColour = HtGoTile::BLACK;

    win.SetFramerateLimit( 60 );
    win.UseVerticalSync( true );

    sf::Sound failSound( soundBuffers["fail-sound"] );

    while( win.IsOpened() ) {
        using namespace std;

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
            case sf::Event::KeyPressed:
                switch( ev.Key.Code ) {
                    case sf::Key::Escape:
                    case sf::Key::Q:
                        win.Close();
                        break;
                    case sf::Key::T:
                        blitter.toggleTGMode();
                        break;
                    case sf::Key::N:
                        blitter.toggleShowNumbers();
                        break;
                    default: break;
                }
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
                } else if( ev.MouseButton.Button == sf::Mouse::Left ) {
                    if( blitter.getTGMode() ) {
                        int x = win.GetInput().GetMouseX(),
                            y = win.GetInput().GetMouseY();
                        if( viewport.translateCoordinates( x, y ) ) {
                            grid.screenToHex( x, y, 0, 0 );
                            if( myTorusMap.putWouldBeLegal( x, y, myCurrentColour ) ) {
                                myTorusMap.put( x, y, myCurrentColour );
                                myCurrentColour = ( myCurrentColour == HtGoTile::BLACK ) ? HtGoTile::WHITE : HtGoTile::BLACK;
                            } else {
                                failSound.Play();
                            }
                        }
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
                        blitter.setSelected( ix, iy );
                        int selectedHexX = ix, selectedHexY = iy;
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
            default: break; // shuts up g++
        }

        win.Clear( sf::Color(0,100,0) );

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
        
        win.Display();
    }

    delete textPopup;

    return 0;
}
