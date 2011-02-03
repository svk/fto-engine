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

#include "mtrand.h"

#include "HexFov.h"

struct Tile {
    enum State {
        FLOOR,
        WALL
    };
    State state;
    Tile(void) : state( Tile::WALL ), lit ( false ) {}
    Tile(State state) : state(state), lit ( false ) {}

    bool lit;
};

class World : public HexTools::HexMap<Tile>,
              public HexTools::HexOpacityMap,
              public HexTools::HexLightReceiver {
    public:
        HexTools::HexFovRegion seen;

        int px, py;

        World(int sz) :
            HexTools::HexMap<Tile>(sz),
            px(0),
            py(0)
        {
            using namespace HexTools;
            MTRand prng ( 1337 );
            int hcs = hexCircleSize(sz);
            get(0,0).state = Tile::FLOOR;
            for(int i=1;i<hcs;i++) {
                int x, y;
                inflateHexCoordinate(i,x,y);
                int ti, tj, tr;
                polariseHexCoordinate( x, y, ti, tj, tr );
                if( tr < sz ) {
                    get(x,y).state = ( prng() > 0.75 ) ? Tile::WALL : Tile::FLOOR;
                } else {
                    get(x,y).state = Tile::WALL;
                }
            }
        }

        void clearlight(void) {
            int sz = getSize();
            for(int i=0;i<sz;i++) {
                get(i).lit = false;
            }
            get(px,py).lit = true;
        }

        bool isOpaque(int x, int y) const {
            return get(x,y).state != Tile::FLOOR;
        }

        void setLit(int x, int y) {
            if( &getDefault() != &get(x,y) ) {
                get(x,y).lit = true;
                seen.add( x, y );
            }
        }

        bool isLit(int x, int y) const {
            return get(x,y).lit;
        }

        bool canMove(int dx, int dy) {
            return ( get(px+dx,py+dy).state == Tile::FLOOR );
        }

        void updateVision(void) {
            clearlight();
            HexTools::HexFov fov ( *this, *this, px, py );
            seen.add( px, py );
            fov.calculate();
        }

        bool move(int dx, int dy) {
            if( get(px+dx,py+dy).state == Tile::FLOOR ) {
                px += dx;
                py += dy;
                updateVision();
                return true;
            }
            return false;
        }
        
};

class LevelBlitter : public HexBlitter {
    private:
        World& world;
        ResourceManager<HexSprite>& sprites;

    public:
        LevelBlitter(World& world, ResourceManager<HexSprite>& sprites) :
            world ( world ),
            sprites ( sprites )
        {
        }


        void drawHex(int x, int y, sf::RenderWindow& win) {
            if( !world.seen.contains(x,y) ) return;
            std::string suffix = ""; // obv unoptimized
            if( !world.get(x,y).lit ) suffix = "-memory";
            switch( world.get(x,y).state ) {
                case Tile::WALL:
                    sprites["tile-wall" + suffix].draw( win );
                    break;
                case Tile::FLOOR:
                    sprites["tile-floor" + suffix].draw( win );
                    break;
            }
            if( !world.get(x,y).lit ) {
                sprites["zone-fog"].draw( win );
            }
        }
};

int main(int argc, char *argv[]) {
    ScreenGrid grid ( "./data/hexproto2.png" );
    ResourceManager<sf::Image> images;
    images.bind( "tile-floor", grid.createSingleColouredImage( sf::Color( 100,200,100 ) ) );
    images.bind( "tile-floor-memory", ToGrayscale().apply(
                                      grid.createSingleColouredImage( sf::Color(100,200,100))));
    images.bind( "tile-wall", grid.createSingleColouredImage( sf::Color( 100,50,50 ) ) );
    images.bind( "tile-wall-memory", ToGrayscale().apply(
                                     grid.createSingleColouredImage( sf::Color(100,50,50))));
    images.bind( "zone-fog", grid.createSingleColouredImage( sf::Color( 0,0,0,128 ) ) );

    ResourceManager<HexSprite> hexSprites;
    hexSprites.bind( "overlay-player", new HexSprite( "./data/smiley32.png", grid ) );
    hexSprites.bind( "tile-wall", new HexSprite( images["tile-wall"], grid ) );
    hexSprites.bind( "tile-wall-memory", new HexSprite( images["tile-wall-memory"], grid ) );
    hexSprites.bind( "tile-floor", new HexSprite( images["tile-floor"], grid ) );
    hexSprites.bind( "tile-floor-memory", new HexSprite( images["tile-floor-memory"], grid ) );
    hexSprites.bind( "zone-fog", new HexSprite( images["zone-fog"], grid ) );

    World world( 40 );
    LevelBlitter levelBlit ( world, hexSprites );
    HexViewport vp ( grid,  0, 0, 640, 480 );
    vp.setBackgroundColour( sf::Color(0,0,0) );
    sf::View mainView ( sf::Vector2f( 0, 0 ),
                        sf::Vector2f( 320, 240 ) );
    sf::RenderWindow win ( sf::VideoMode(640,480,32), "Hexplorer demo" );

    const double transitionTime = 0.15;
    bool transitioning = false;
    bool drybump;
    double transitionPhase;
    int tdx, tdy;

    sf::Clock clock;
    win.SetView( mainView );
    win.SetFramerateLimit( 30 );

    world.updateVision();

    while( win.IsOpened() ) {
        double dt = clock.GetElapsedTime();
        clock.Reset();

        sf::Event ev;

        if( transitioning ) {
            transitionPhase += dt;
            if( transitionPhase >= transitionTime ) {
                if( !drybump ) {
                    world.move( tdx, tdy );
                }
                transitioning = false;
            }
        }

        while( win.GetEvent( ev ) ) switch( ev.Type ) {
            default: break;
            case sf::Event::Resized:
                using namespace std;
                vp.setRectangle(0, 0, win.GetWidth(), win.GetHeight() );
                mainView.SetHalfSize( (double)ev.Size.Width/2.0, (double)ev.Size.Height/2.0 );
                break;
            case sf::Event::Closed:
                win.Close();
                break;
            case sf::Event::MouseButtonPressed:
                if( ev.MouseButton.Button == sf::Mouse::Left ) {
                    int x = win.GetInput().GetMouseX(),
                        y = win.GetInput().GetMouseY();
                    if( vp.translateCoordinates( x, y ) ) {
                        grid.screenToHex( x, y, 0, 0 );
                        if( x == world.px && y == world.py ) break;
                        if( !world.get(x,y).lit ) break;
                        if( (&world.getDefault()) != (&world.get(x,y)) ) {
                            world.get(x,y).state = (world.get(x,y).state == Tile::FLOOR) ? Tile::WALL : Tile::FLOOR;
                            world.updateVision();
                        }
                    }
                }
            case sf::Event::KeyPressed:
                if( transitioning ) break;
                transitioning = true;
                transitionPhase = 0.0;
                switch( ev.Key.Code ) {
                    case sf::Key::Q: tdx = -3; tdy = 1; break;
                    case sf::Key::W: tdx = 0; tdy = 2; break;
                    case sf::Key::E: tdx = 3; tdy = 1; break;
                    case sf::Key::A: tdx = -3; tdy = -1; break;
                    case sf::Key::S: tdx = 0; tdy = -2; break;
                    case sf::Key::D: tdx = 3; tdy = -1; break;
                    default: transitioning = false; break;
                }
                if( transitioning && !world.canMove( tdx, tdy ) ) {
                    transitioning = false;
                }
                if( transitioning && ev.Key.Shift ) {
                    drybump = true;
                } else {
                    drybump = false;
                }
                break;
        }
        int rtdx = tdx, rtdy = tdy;
        int tdxv = 0, tdyv = 0;
        if( transitioning ) {
            grid.hexToScreen( rtdx, rtdy );
            tdxv = rtdx * (transitionPhase / transitionTime);
            tdyv = rtdy * (transitionPhase / transitionTime);
        }
        int cx = world.px, cy = world.py;
        grid.hexToScreen( cx, cy );
        vp.center( cx, cy );
        cx += tdxv;
        cy += tdyv;
        if( !drybump ) {
            vp.center( cx, cy );
        }

        win.Clear(sf::Color(255,0,255));
        vp.draw( levelBlit, win, mainView );

//        mainView.SetCenter( 0, 0 );

        vp.beginClip( win.GetWidth(), win.GetHeight() );
        vp.translateToHex( world.px, world.py, win.GetWidth(), win.GetHeight(), mainView );
        if( transitioning ) {
            mainView.Move( -tdxv, -tdyv );
        }
        hexSprites["overlay-player"].draw( win );
        vp.endClip();

        win.Display();
    }
}
