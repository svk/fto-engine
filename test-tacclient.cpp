#include "mtrand.h"

#include "HexFov.h"
#include "TacClient.h"
#include "TacClientAction.h"

void getVisionRegion(const HexTools::HexMap<bool>& smap, int x, int y, HexTools::HexFovRegion& region) {
    using namespace HexTools;
    using namespace Tac;
    struct OpacMap : public HexOpacityMap {
        const HexMap<bool>& smap;

        OpacMap(const HexMap<bool>& smap) : smap(smap) {}
        bool isOpaque(int x, int y) const {
            return smap.get(x,y);
        }
    };
    OpacMap smapopac ( smap );
    HexFov fov ( smapopac, region, x, y );
    fov.calculate();
}

void updateVision(HexTools::HexMap<bool>& smap, Tac::ClientMap& cmap, ResourceManager<Tac::ClientTileType>& tileTypes, int x, int y) {
    using namespace HexTools;
    using namespace Tac;
    HexFovRegion region;
    getVisionRegion( smap, x, y, region );
    for(HexRegion::const_iterator i = region.begin(); i != region.end(); i++) {
        using namespace std;
        if( smap.get(i->first, i->second) ) {
            cmap.setTileType( i->first, i->second, &tileTypes[ "wall" ] );
        } else {
            cmap.setTileType( i->first, i->second, &tileTypes[ "floor" ] );
        }
    }
    cmap.updateActive( region );
}

int main(int argc, char *argv[]) {
    using namespace HexTools;
    using namespace Tac;
    using namespace std;

    const int mapSize = 40;

    SpriteId::bindAlias( "unit-smiley", 0 );
    SpriteId::bindAlias( "unit-troll", 1 );
    SpriteId::bindAlias( "tile-floor", 2 );
    SpriteId::bindAlias( "tile-wall", 3 );
    SpriteId::bindAlias( "zone-fog", 4 );
    SpriteId::bindAlias( "zone-move", 5 );
    SpriteId::bindAlias( "zone-attack", 6 );
    SpriteId::bindAlias( "grid-thin", 7 );

    ScreenGrid grid ( "./data/hexproto2.png" );
    TacSpritesheet sheet ( 1024, 1024 );

    sheet.adopt( SpriteId( "unit-smiley", SpriteId::NORMAL ),
                 loadImageFromFile( "./data/smiley32.png" ) );
    sheet.adopt( SpriteId( "tile-floor", SpriteId::NORMAL ),
                 grid.createSingleColouredImage( sf::Color( 100,200,100 ) ) );
    sheet.adopt( SpriteId( "tile-floor", SpriteId::GRAYSCALE ),
                 ToGrayscale().apply( grid.createSingleColouredImage( sf::Color( 100,200,100 ) ) ) );
    sheet.adopt( SpriteId( "tile-wall", SpriteId::NORMAL ),
                 grid.createSingleColouredImage( sf::Color( 50,20,20 ) ) );
    sheet.adopt( SpriteId( "tile-wall", SpriteId::GRAYSCALE ),
                 ToGrayscale().apply( grid.createSingleColouredImage( sf::Color( 50,20,20 ) ) ) );
    sheet.adopt( SpriteId("zone-fog", SpriteId::NORMAL),
                 grid.createSingleColouredImage( sf::Color( 0,0,0,128 ) ) );
    sheet.adopt( SpriteId("zone-attack", SpriteId::NORMAL),
                 grid.createSingleColouredImage( sf::Color( 255,0,0,128 ) ) );
    sheet.adopt( SpriteId("zone-move", SpriteId::NORMAL),
                 grid.createSingleColouredImage( sf::Color( 0,255,0,128 ) ) );
    sheet.adopt( SpriteId("grid-thin", SpriteId::NORMAL),
                 loadImageFromFile( "./data/hexthingrid2.png" ) );

    ResourceManager<ClientTileType> tileTypes;
    ResourceManager<ClientUnitType> unitTypes;

    unitTypes.bind( "player", new ClientUnitType( sheet, "unit-smiley", "Player" ) );

    tileTypes.bind( "border", new ClientTileType( sheet, "tile-wall", "hard wall", Type::WALL, Type::BLOCK, true, 0 ) );
    tileTypes.bind( "floor", new ClientTileType( sheet, "tile-floor", "floor", Type::FLOOR, Type::CLEAR, false, 100 ) );
    tileTypes.bind( "wall", new ClientTileType( sheet, "tile-wall", "wall", Type::WALL, Type::BLOCK, false, 0 ) );

    const int playerId = 1;
    const int playerTeam = 0, playerNo = 0;

    int playerX = 3, playerY = 1; // keeping track of pretenses, this is "server side"

    ClientMap cmap ( mapSize, sheet, grid );
    cmap.adoptUnit( new ClientUnit( playerId, unitTypes["player"], playerTeam, playerNo ) );
    cmap.placeUnitAt( playerId, playerX, playerY, 0 );

    MTRand prng ( 1337 );
    HexMap<bool> smap ( mapSize );
    for(int r=1;r<mapSize;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++) {
        int x, y;
        cartesianiseHexCoordinate( i, j, r, x, y );
        smap.get(x,y) = prng() > 0.5;
    }
    smap.get(0,0) = false;
    smap.get(playerX,playerY) = false;

    updateVision( smap, cmap, tileTypes, playerX, playerY );

    const int winWidth = 640, winHeight = 480;
    sf::RenderWindow win ( sf::VideoMode( winWidth ,winHeight,32), "TacClient demo" );
    HexViewport vp ( grid,  0, 0, winWidth, winHeight );

    win.SetFramerateLimit( 30 );
    while( win.IsOpened() ) {
        cmap.animate( win.GetFrameTime() );
        cmap.processActions();

        sf::Event ev;
        while( win.GetEvent( ev ) ) switch( ev.Type ) {
            default: break;
            case sf::Event::KeyPressed:
                {
                    int mx = 0, my = 0;
                    switch( ev.Key.Code ) {
                        case sf::Key::Q: mx = -3; my = 1; break;
                        case sf::Key::W: mx = 0; my = 2; break;
                        case sf::Key::E: mx = 3; my = 1; break;
                        case sf::Key::A: mx = -3; my = -1; break;
                        case sf::Key::S: mx = 0; my = -2; break;
                        case sf::Key::D: mx = 3; my = -1; break;
                        default: break;
                    }
                    if( cmap.unitMayMoveTo( playerId, playerX + mx, playerY + my ) ) { // this check should be done both ss and cs
                        cmap.queueAction( new BumpAnimationCAction( cmap, playerId, mx, my ) );
                        cmap.queueAction( new NormalMovementCAction( cmap, playerId, mx, my ) );
                        playerX += mx;
                        playerY += my; // position immediately updated serverside
                        HexFovRegion newVision;
                        getVisionRegion( smap, playerX, playerY, newVision );
                        RevealTerrainCAction *rev = new RevealTerrainCAction( cmap );
                        SetActiveRegionCAction *act = new SetActiveRegionCAction( cmap );
                        // for now we just Send. Everything. wrt terrain revelations
                        for(HexFovRegion::const_iterator i = newVision.begin(); i != newVision.end(); i++) {
                            if( smap.get(i->first, i->second) ) {
                                rev->add( i->first, i->second, &tileTypes["wall"] );
                            } else {
                                rev->add( i->first, i->second, &tileTypes["floor"] );
                            }
                            act->add( i->first, i->second );
                        }
                        cmap.queueAction( rev );
                        cmap.queueAction( act );
                    }
                }
                break;
            case sf::Event::Resized:
                using namespace std;
                vp.setRectangle(0, 0, win.GetWidth(), win.GetHeight() );
                break;
            case sf::Event::Closed:
                win.Close();
                break;
        }
        double cx, cy;
        if( cmap.getUnitScreenPositionById( playerId, cx, cy ) ) {
            vp.center( cx, cy );
        }

        win.Clear( sf::Color(0,0,0) );

        glViewport( 0, 0, win.GetWidth(), win.GetHeight() );

        glMatrixMode( GL_PROJECTION );
        glLoadIdentity();

        glMatrixMode( GL_MODELVIEW );
        glLoadIdentity();
        sf::Matrix3 myMatrix;

        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        sheet.bindTexture();

        vp.drawGL( cmap.getLevelBlitter(), win, win.GetWidth(), win.GetHeight() );
        vp.drawGL( cmap.getUnitBlitter(0), win, win.GetWidth(), win.GetHeight() );

        win.Display();
    }
    
    return 0;
}