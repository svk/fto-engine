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
                get(x,y).state = ( prng() > 0.5 ) ? Tile::WALL : Tile::FLOOR;
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
            }
        }

        bool isLit(int x, int y) const {
            return get(x,y).lit;
        }

        bool canMove(int dx, int dy) {
            return ( get(px+dx,py+dy).state == Tile::FLOOR );
        }

        bool move(int dx, int dy) {
            if( get(px+dx,py+dy).state == Tile::FLOOR ) {
                px += dx;
                py += dy;

                clearlight();
                HexTools::HexFov fov ( *this, *this, px, py );
                fov.calculate();

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
            if( !world.get(x,y).lit ) return;
            switch( world.get(x,y).state ) {
                case Tile::WALL:
                    sprites["tile-wall"].draw( win );
                    break;
                case Tile::FLOOR:
                    sprites["tile-floor"].draw( win );
                    break;
            }
        }
};

class MobBlitter : public HexBlitter {
    private:
        ResourceManager<HexSprite>& sprites;

        int px, py;
    public:
        MobBlitter(ResourceManager<HexSprite>& sprites, int px, int py) : sprites(sprites),px(px),py(py) {}

        void drawHex(int x, int y, sf::RenderWindow& win) {
            if( x == px && y == py ) {
                sprites["overlay-player"].draw( win );
            }
        }
};

int main(int argc, char *argv[]) {
    ScreenGrid grid ( "./data/hexproto2.png" );
    ResourceManager<sf::Image> images;
    images.bind( "tile-floor", grid.createSingleColouredImage( sf::Color( 100,200,100 ) ) );
    images.bind( "tile-wall", grid.createSingleColouredImage( sf::Color( 150,100,100 ) ) );

    ResourceManager<HexSprite> hexSprites;
    hexSprites.bind( "overlay-player", new HexSprite( "./data/smiley32.png", grid ) );
    hexSprites.bind( "tile-wall", new HexSprite( images["tile-wall"], grid ) );
    hexSprites.bind( "tile-floor", new HexSprite( images["tile-floor"], grid ) );

    World world( 40 );
    LevelBlitter levelBlit ( world, hexSprites );
    HexViewport vp ( grid,  0, 0, 640, 480 );
    sf::View mainView ( sf::Vector2f( 0, 0 ),
                        sf::Vector2f( 320, 240 ) );
    sf::RenderWindow win ( sf::VideoMode(640,480,32), "Hexplorer demo" );

    const double transitionTime = 0.15;
    bool transitioning = false;
    double transitionPhase;
    int tdx, tdy;

    sf::Clock clock;
    win.SetView( mainView );
    win.SetFramerateLimit( 30 );

    while( win.IsOpened() ) {
        double dt = clock.GetElapsedTime();
        clock.Reset();

        sf::Event ev;

        if( transitioning ) {
            transitionPhase += dt;
            if( transitionPhase >= transitionTime ) {
                world.move( tdx, tdy );
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
        cx += tdxv;
        cy += tdyv;
        vp.center( cx, cy );

        win.Clear(sf::Color(255,0,255));
        vp.draw( levelBlit, win, mainView );
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
