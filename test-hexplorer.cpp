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

struct Tile {
    enum State {
        FLOOR,
        WALL
    };
    State state;
    Tile(State state) : state(state) {}
};

class Semimap {
    private:
        Tile wall, floor;

    public:
        Semimap(void) : wall(Tile::WALL), floor(Tile::FLOOR) {}

        Tile& get(int x,int y) {
            if( x == 0 && y == 0 ) return floor;
            srand( 1337 + x * 1337 + y ); // hax
            switch( rand() % 2 ) {
                case 1: return floor;
                case 0: return wall;
            }
            return wall;
        }
};

class World : public HexBlitter {
    private:
        Semimap semi;
        ResourceManager<HexSprite>& sprites;

    public:
        int px, py;

        World(ResourceManager<HexSprite>& sprites) :
            sprites ( sprites ),
            px( 0 ),
            py( 0 )
        {
        }

        bool move(int dx, int dy) {
            if( semi.get(px+dx,py+dy).state == Tile::FLOOR ) {
                px += dx;
                py += dy;
                return true;
            }
            return false;
        }

        void drawHex(int x, int y, sf::RenderWindow& win) {
            if( px == x && py == y ) {
                sprites["tile-player"].draw( win );
                return;
            }
            switch( semi.get(x,y).state ) {
                case Tile::WALL:
                    sprites["tile-wall"].draw( win );
                    break;
                case Tile::FLOOR:
                    sprites["tile-floor"].draw( win );
                    break;
            }
        }
};

int main(int argc, char *argv[]) {
    ScreenGrid grid ( "./data/hexproto2.png" );
    ResourceManager<HexSprite> hexSprites;
    hexSprites.bind( "tile-player", new HexSprite( "./data/hexgray2.png", grid ) );
    hexSprites.bind( "tile-wall", new HexSprite( "./data/hexrainbow2.png", grid ) );
    hexSprites.bind( "tile-floor", new HexSprite( "./data/hexwhite2.png", grid ) );
    World world ( hexSprites );
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
                vp.setRectangle(100, 100, win.GetWidth()-200, win.GetHeight()-200 );
                mainView.SetHalfSize( ev.Size.Width/2, ev.Size.Height/2 );
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
                break;
        }
        win.Clear(sf::Color(255,0,255));
        vp.draw( world, win, mainView );
        win.Display();
    }
}