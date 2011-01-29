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

struct Tile {
    enum State {
        FLOOR,
        WALL
    };
    State state;
    Tile(void) : state( Tile::WALL ) {}
    Tile(State state) : state(state) {}
};

class World : public HexTools::HexMap<Tile> {
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

        bool move(int dx, int dy) {
            if( get(px+dx,py+dy).state == Tile::FLOOR ) {
                px += dx;
                py += dy;
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
    ResourceManager<HexSprite> hexSprites;
    hexSprites.bind( "overlay-player", new HexSprite( "./data/smiley32.png", grid ) );
    hexSprites.bind( "tile-wall", grid.createSingleColouredSprite( sf::Color(150,100,100) ) );
    hexSprites.bind( "tile-floor", grid.createSingleColouredSprite( sf::Color(100,200,100) ) );
    World world( 40 );
    LevelBlitter levelBlit ( world, hexSprites );
    HexViewport vp ( grid,  0, 0, 640, 480 );
    sf::View mainView ( sf::Vector2f( 0, 0 ),
                        sf::Vector2f( 320, 240 ) );
    sf::RenderWindow win ( sf::VideoMode(640,480,32), "Hexplorer demo" );

    win.SetView( mainView );
    win.SetFramerateLimit( 30 );

    while( win.IsOpened() ) {
        sf::Event ev;
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
                switch( ev.Key.Code ) {
                    case sf::Key::Q: world.move(-3,1); break;
                    case sf::Key::W: world.move(0,2); break;
                    case sf::Key::E: world.move(3,1); break;
                    case sf::Key::A: world.move(-3,-1); break;
                    case sf::Key::S: world.move(0,-2); break;
                    case sf::Key::D: world.move(3,-1); break;
                    default: break;
                }
                int ax = world.px, ay = world.py;
                grid.hexToScreen( ax, ay );
                vp.center( ax, ay );
                break;
        }
        win.Clear(sf::Color(255,0,255));
        vp.draw( levelBlit, win, mainView );
        MobBlitter mobs ( hexSprites, world.px, world.py );
        vp.draw( mobs, win, mainView );
        win.Display();
    }
}
