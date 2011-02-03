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
        KeyedSpritesheet& sheet;

        sf::Sprite tileWall, tileFloor, zoneFog, tileWallMemory, tileFloorMemory;
        sf::Sprite thinGrid;

    public:
        LevelBlitter(World& world, ResourceManager<HexSprite>& sprites, KeyedSpritesheet& sheet) :
            world ( world ),
            sprites ( sprites ),
            sheet ( sheet ),
            tileWall ( sheet.makeSpriteNamed( "tile-wall" ) ),
            tileFloor ( sheet.makeSpriteNamed( "tile-floor" ) ),
            zoneFog ( sheet.makeSpriteNamed( "zone-fog" ) ),
            tileWallMemory ( sheet.makeSpriteNamed( "tile-wall-memory" ) ),
            tileFloorMemory ( sheet.makeSpriteNamed( "tile-floor-memory" ) ),
            thinGrid ( sheet.makeSpriteNamed( "thin-grid" ) )
        {
        }

        void putSprite(const sf::Sprite& sprite) {
            const float width = sprite.GetSize().x, height = sprite.GetSize().y;
            const sf::FloatRect rect = sprite.GetImage()->GetTexCoords( sprite.GetSubRect() );
            glBegin( GL_QUADS );
            glTexCoord2f( rect.Left, rect.Top ); glVertex2f(0+0.5,0+0.5);
            glTexCoord2f( rect.Left, rect.Bottom ); glVertex2f(0+0.5,height+0.5);
            glTexCoord2f( rect.Right, rect.Bottom ); glVertex2f(width+0.5,height+0.5);
            glTexCoord2f( rect.Right, rect.Top ); glVertex2f(width+0.5,0+0.5);
            glEnd();
        }


        void drawHex(int x, int y, sf::RenderWindow& win) {
            if( !world.seen.contains(x,y) ) return;
            if( world.get(x,y).lit ) switch( world.get(x,y).state ) {
                case Tile::WALL:
                    putSprite( tileWall );
                    break;
                case Tile::FLOOR:
                    putSprite( tileFloor );
                    break;
            } else switch( world.get(x,y).state ) {
                case Tile::WALL:
                    putSprite( tileWallMemory );
                    break;
                case Tile::FLOOR:
                    putSprite( tileFloorMemory );
                    break;
            }
            putSprite( thinGrid );
        }
};

int main(int argc, char *argv[]) {
    ScreenGrid grid ( "./data/hexproto2.png" );

    KeyedSpritesheet images (1024,1024);
    images.adoptAs( "smiley", loadImageFromFile( "./data/smiley32.png" ) );
    images.adoptAs( "tile-floor", grid.createSingleColouredImage( sf::Color( 100,200,100 ) ) );
    images.adoptAs( "tile-floor-memory", ToGrayscale().apply(
                                      grid.createSingleColouredImage( sf::Color(100,200,100))));
    images.adoptAs( "tile-wall", grid.createSingleColouredImage( sf::Color( 100,50,50 ) ) );
    images.adoptAs( "tile-wall-memory", ToGrayscale().apply(
                                     grid.createSingleColouredImage( sf::Color(100,50,50))));
    images.adoptAs( "zone-fog", grid.createSingleColouredImage( sf::Color( 0,0,0,128 ) ) );
    images.adoptAs( "thin-grid", loadImageFromFile( "./data/hexthingrid2.png" ) );

    ResourceManager<HexSprite> hexSprites;
    hexSprites.bind( "overlay-player", new HexSprite( images.makeSpriteNamed( "smiley"), grid ) );
    hexSprites.bind( "tile-wall", new HexSprite( images.makeSpriteNamed( "tile-wall" ), grid ) );
    hexSprites.bind( "tile-wall-memory", new HexSprite( images.makeSpriteNamed( "tile-wall-memory" ), grid ) );
    hexSprites.bind( "tile-floor", new HexSprite( images.makeSpriteNamed( "tile-floor" ), grid ) );
    hexSprites.bind( "tile-floor-memory", new HexSprite( images.makeSpriteNamed( "tile-floor-memory" ), grid ) );
    hexSprites.bind( "zone-fog", new HexSprite( images.makeSpriteNamed( "zone-fog" ), grid ) );

    sf::Image *testImage, *testImage2;
    testImage = loadImageFromFile( "./data/smiley32.png" );
    testImage2 = loadImageFromFile( "./data/hexproto2.png" );

    World world( 40 );
    LevelBlitter levelBlit ( world, hexSprites, images );
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
        using namespace std;

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


        win.Clear(sf::Color(255,0,0));

        glViewport( 0, 0, win.GetWidth(), win.GetHeight() );

        glMatrixMode( GL_PROJECTION );
        glLoadIdentity();

        glMatrixMode( GL_MODELVIEW );
        glLoadIdentity();
        sf::Matrix3 myMatrix;
        myMatrix(0, 0) = 1.f / (mainView.GetHalfSize().x);
        myMatrix(1, 1) = -1.f / (mainView.GetHalfSize().y);
        myMatrix(0, 2) = 100 / (mainView.GetHalfSize().x);
        myMatrix(1, 2) = 0 / (mainView.GetHalfSize().y);
        glLoadMatrixf( myMatrix.Get4x4Elements() );

        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        images.bindTexture();

        vp.drawGL( levelBlit, win, win.GetWidth(), win.GetHeight() );

        mainView.SetCenter( 0, 0 );

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
