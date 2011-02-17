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

#include "TacDungeon.h"

struct Tile {
    enum State {
        FLOOR,
        WALL,
        DOOR,
        HL1,
        HL2
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
            return !(get(x,y).state == Tile::FLOOR
                     || ( get(x,y).state == Tile::HL1 ) );
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
            return ( get(px+dx,py+dy).state == Tile::FLOOR )
                   || ( get(px+dx,py+dy).state == Tile::HL1 )
                   || ( get(px+dx,py+dy).state == Tile::HL2 )
                   ;
        }

        void updateVision(void) {
            clearlight();
            HexTools::HexFov fov ( *this, *this, px, py );
            seen.add( px, py );
            fov.calculate();
        }

        bool move(int dx, int dy) {
            if( get(px+dx,py+dy).state == Tile::FLOOR
                || get(px+dx,py+dy).state == Tile::HL1
                || get(px+dx,py+dy).state == Tile::HL2
                ) {
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
        StringKeyedSpritesheet& sheet;

        sf::Sprite tileWall, tileFloor, zoneFog, tileWallMemory, tileFloorMemory, tileDoor;
        sf::Sprite thinGrid;
        sf::Sprite zoneRed, zoneGreen;

        HexTools::HexRegion regionGreen, regionRed;

    public:
        LevelBlitter(World& world, ResourceManager<HexSprite>& sprites, StringKeyedSpritesheet& sheet) :
            world ( world ),
            sprites ( sprites ),
            sheet ( sheet ),
            tileWall ( sheet.makeSprite( "tile-wall" ) ),
            tileFloor ( sheet.makeSprite( "tile-floor" ) ),
            zoneFog ( sheet.makeSprite( "zone-fog" ) ),
            tileWallMemory ( sheet.makeSprite( "tile-wall-memory" ) ),
            tileFloorMemory ( sheet.makeSprite( "tile-floor-memory" ) ),
            tileDoor ( sheet.makeSprite( "tile-door" ) ),
            thinGrid ( sheet.makeSprite( "thin-grid" ) ),
            zoneRed ( sheet.makeSprite( "zone-red" ) ),
            zoneGreen ( sheet.makeSprite( "zone-green" ) )
        {
        }

        void updateRegions(int px, int py) {
            regionRed.clear();
            regionGreen.clear();
            for(int r=1;r<=2;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++) {
                int x, y;
                HexTools::cartesianiseHexCoordinate( i, j, r, x, y );
                x += px;
                y += py;
                if( world.get(x,y).state == Tile::WALL ) continue;
                if( r == 1 ) regionGreen.add( x, y );
                else regionRed.add( x, y );
            }
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
            const bool rrContains = regionRed.contains( x, y ),
                       rgContains = regionGreen.contains( x, y ),
                       isLit = world.get(x,y).lit;
            if( !world.seen.contains(x,y) ) return;
            if( world.get(x,y).state == Tile::WALL ) return;
            if( isLit ) switch( world.get(x,y).state ) {
                case Tile::HL1:
                    putSprite( zoneRed );
                    break;
                case Tile::HL2:
                    putSprite( zoneGreen );
                    break;
                case Tile::DOOR:
                    putSprite( tileDoor );
                    break;
                case Tile::WALL:
                    putSprite( tileWall );
                    break;
                case Tile::FLOOR:
                    putSprite( tileFloor );
                    break;
            } else switch( world.get(x,y).state ) {
                default:
                case Tile::HL2:
                case Tile::WALL:
                    putSprite( tileWallMemory );
                    break;
                case Tile::HL1:
                case Tile::FLOOR:
                    putSprite( tileFloorMemory );
                    break;
            }
#if 0
            if( !isLit ) {
                putSprite( zoneFog );
            } else if( rgContains ) {
                putSprite( zoneGreen );
            } else if( rrContains ) {
                putSprite( zoneRed );
            }
#endif
            putSprite( thinGrid );
        }
};

int main(int argc, char *argv[]) {
    using namespace Tac;
    FreetypeLibrary ftLib;
    FreetypeFace ftFont ("./data/BienetBold.ttf", 20);

    ScreenGrid grid ( "./data/hexproto2.png" );

    StringKeyedSpritesheet images (1024,1024);
    images.adopt( "smiley", loadImageFromFile( "./data/smiley32.png" ) );
    images.adopt( "tile-floor", grid.createSingleColouredImage( sf::Color( 100,200,100 ) ) );
    images.adopt( "tile-floor-memory", ToGrayscale().apply(
                                      grid.createSingleColouredImage( sf::Color(100,200,100))));
    images.adopt( "tile-wall", grid.createSingleColouredImage( sf::Color( 50,20,20 ) ) );
    images.adopt( "tile-door", grid.createSingleColouredImage( sf::Color( 200,100,100 ) ) );
    images.adopt( "tile-wall-memory", ToGrayscale().apply(
                                     grid.createSingleColouredImage( sf::Color(50,20,20))));
    images.adopt( "zone-fog", grid.createSingleColouredImage( sf::Color( 0,0,0,128 ) ) );
    images.adopt( "zone-red", grid.createSingleColouredImage( sf::Color( 255,0,0,128 ) ) );
    images.adopt( "zone-green", grid.createSingleColouredImage( sf::Color( 0,255,0,128 ) ) );
    images.adopt( "thin-grid", loadImageFromFile( "./data/hexthingrid2.png" ) );

    ResourceManager<HexSprite> hexSprites;
    hexSprites.bind( "overlay-player", new HexSprite( images.makeSprite( "smiley"), grid ) );
    hexSprites.bind( "tile-wall", new HexSprite( images.makeSprite( "tile-wall" ), grid ) );
    hexSprites.bind( "tile-wall-memory", new HexSprite( images.makeSprite( "tile-wall-memory" ), grid ) );
    hexSprites.bind( "tile-floor", new HexSprite( images.makeSprite( "tile-floor" ), grid ) );
    hexSprites.bind( "tile-floor-memory", new HexSprite( images.makeSprite( "tile-floor-memory" ), grid ) );
    hexSprites.bind( "zone-fog", new HexSprite( images.makeSprite( "zone-fog" ), grid ) );
    hexSprites.bind( "zone-red", new HexSprite( images.makeSprite( "zone-red" ), grid ) );
    hexSprites.bind( "zone-green", new HexSprite( images.makeSprite( "zone-green" ), grid ) );

    MTRand_int32 prng ( time(0) );
    SimpleLevelGenerator *levelgen = 0;
    while( !levelgen ) {
        try {
            levelgen = new SimpleLevelGenerator( prng );
            levelgen->setRoomTarget( 5 );
            levelgen->setShortestCorridorsFirst();
            levelgen->setStopWhenConnected();
            levelgen->setSWCExtraCorridors( 2 );

            levelgen->adoptPainter( new Tac::HollowHexagonRoomPainter( 6, 2 ),
                                   1,
                                   false );
            levelgen->adoptPainter( new Tac::HollowHexagonRoomPainter( 7, 3 ),
                                   1,
                                   false );
            levelgen->adoptPainter( new Tac::BlankRoomPainter( 3 ),
                                   1,
                                   false );

            levelgen->adoptPainter( new Tac::HexagonRoomPainter( 4 ),
                                   2,
                                   true );
            levelgen->adoptPainter( new Tac::HexagonRoomPainter( 5 ),
                                   2,
                                   true );
            levelgen->adoptPainter( new Tac::HexagonRoomPainter( 6 ),
                                   2,
                                   true );
            levelgen->adoptPainter( new Tac::HexagonRoomPainter( 7 ),
                                   1,
                                   true );

            levelgen->generate();
        }
        catch( LevelGenerationFailure& e ) {
            using namespace std;
            cerr << "warning: level generation failure, trying again.. (" << e.what() << ")" << endl;
            delete levelgen;
            levelgen = 0;
        }
    }

    DungeonSketch& sketch = levelgen->getSketch();
    int maxr = sketch.getMaxRadius();

    World world( maxr );
    LevelBlitter levelBlit ( world, hexSprites, images );
    HexViewport vp ( grid,  0, 0, 640, 480 );
    vp.setBackgroundColour( sf::Color(0,0,0) );
    sf::View mainView ( sf::Vector2f( 0, 0 ),
                        sf::Vector2f( 320, 240 ) );
    sf::RenderWindow win ( sf::VideoMode(640,480,32), "Hexplorer demo" );

    
    for(int r=0;r<=maxr;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++) {
        int x, y;
        HexTools::cartesianiseHexCoordinate( i, j, r, x, y );
        switch( sketch.get( x, y ) ) {
            case DungeonSketch::ST_NORMAL_FLOOR:
                world.get( x, y ) = Tile::FLOOR;
                break;
            case DungeonSketch::ST_NORMAL_DOORWAY:
                if( prng(0,1) ) {
                    world.get( x, y ) = Tile::HL2;
                } else {
                    world.get( x, y ) = Tile::HL1;
                }
                break;
            case DungeonSketch::ST_NORMAL_CORRIDOR:
                world.get( x, y ) = Tile::HL1;
                break;
            case DungeonSketch::ST_META_CONNECTOR:
            case DungeonSketch::ST_NONE:
            case DungeonSketch::ST_META_DIGGABLE:
            default:
                world.get( x, y ) = Tile::WALL;
                break;
        }
    }

    delete levelgen;

    const double transitionTime = 0.15;
    bool transitioning = false;
    bool drybump;
    double transitionPhase;
    int tdx, tdy;

    sf::Clock clock;
    win.SetView( mainView );
    win.SetFramerateLimit( 30 );


    typedef FiniteLifetimeObjectList<RisingTextAnimation> RTAManager;
    RTAManager textAnims;

    world.updateVision();
    levelBlit.updateRegions(0,0);

    ViewportMouseScroller *vpMouseScroller = 0;

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
                    levelBlit.updateRegions( world.px, world.py );
                } else {
                    int ax = world.px + tdx, ay = world.py + tdy;
                    vp.hexToScreen( ax, ay );
                    LabelSprite *label = new LabelSprite( (prng(0,1)) ? "42" : "*miss*", sf::Color(200,0,0), ftFont );
                    ax -= label->getWidth() / 2.0;
                    textAnims.adopt(
                        new RisingTextAnimation( ax, ay, label, 1.0, 100.0 )
                    );
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
            case sf::Event::MouseMoved:
                if( vpMouseScroller ) {
                    vpMouseScroller->scroll();
                }
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
                            levelBlit.updateRegions(world.px,world.py);
                        }
                    }
                } else if( ev.MouseButton.Button == sf::Mouse::Right ) {
                    using namespace std;
                    if( !vpMouseScroller ) {
                        win.ShowMouseCursor( false );
                        vpMouseScroller = new ViewportMouseScroller( vp, win.GetInput() );
                    }
                }
                break;
            case sf::Event::MouseButtonReleased:
                if( ev.MouseButton.Button == sf::Mouse::Right ) {
                    if( vpMouseScroller ) {
                        delete vpMouseScroller;
                        vpMouseScroller = 0;
                        win.ShowMouseCursor( true );
                    }
                }
                break;
            case sf::Event::KeyPressed:
                if( transitioning ) break;
                transitioning = true;
                transitionPhase = 0.0;
                switch( ev.Key.Code ) {
                    case sf::Key::M:
                        win.Close();
                        transitioning = false;
                        break;
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
//        vp.center( cx, cy );
        cx += tdxv;
        cy += tdyv;
        if( !drybump ) {
//            vp.center( cx, cy );
            ;
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
        for(RTAManager::List::iterator i = textAnims.objects.begin(); i != textAnims.objects.end(); i++) {
            (*i)->animate( dt );
            win.Draw( **i );
        }
        textAnims.prune();

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
