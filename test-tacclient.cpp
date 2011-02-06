#include "mtrand.h"

#include "typesetter.h"
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

    FreetypeLibrary lib;
    FreetypeFace risingTextFont ("./data/CrimsonText-Bold.otf", 20);

    const int mapSize = 40;

    ScreenGrid grid ( "./data/hexproto2.png" );

    TacSpritesheet sheet ( 1024, 1024 );
    loadSpritesFromFile( "./config/sprites.lisp", sheet, grid );

    ResourceManager<RandomVariantsCollection<sf::SoundBuffer> > soundBuffers;
    loadSoundsFromFile( "./config/sound-effects.lisp", soundBuffers );

    ResourceManager<ClientTileType> tileTypes;
    ResourceManager<ClientUnitType> unitTypes;

    unitTypes.bind( "player", new ClientUnitType( sheet, "unit-smiley", "Player" ) );
    unitTypes.bind( "troll", new ClientUnitType( sheet, "unit-troll", "Troll" ) );

    tileTypes.bind( "border", new ClientTileType( sheet, "border", "tile-wall", "hard wall", Type::WALL, Type::BLOCK, true, 0 ) );
    tileTypes.bind( "floor", new ClientTileType( sheet, "floor", "tile-floor", "floor", Type::FLOOR, Type::CLEAR, false, 100 ) );
    tileTypes.bind( "wall", new ClientTileType( sheet, "wall", "tile-wall", "wall", Type::WALL, Type::BLOCK, false, 0 ) );

    const int playerId = 1;
    const int playerTeam = 0, playerNo = 0;

    int nextId = playerId + 1;

    int playerX = 3, playerY = 1; // keeping track of pretenses, this is "server side"

    ClientMap cmap ( mapSize, sheet, grid, &risingTextFont );
    cmap.adoptUnit( new ClientUnit( playerId, unitTypes["player"], playerTeam, playerNo ) );
    cmap.placeUnitAt( playerId, playerX, playerY, 0 );


    MTRand prng ( 1337 );
    HexMap<bool> smap ( mapSize );
    for(int r=1;r<=mapSize;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++) {
        int x, y;
        cartesianiseHexCoordinate( i, j, r, x, y );
        smap.get(x,y) = prng() < 0.4;
    }
    smap.get(0,0) = false;
    smap.get(playerX,playerY) = false;
    smap.getDefault() = true;

    updateVision( smap, cmap, tileTypes, playerX, playerY );

    HexFovRegion nonmonsterspawnRegion;
    getVisionRegion( smap, playerX, playerY, nonmonsterspawnRegion );
    int trollX = 0, trollY = 0;
    int trollId = nextId++;
    bool trollLives = true;
    MTRand_int32 iprng (1337+1);
        // not a very classy way to do this, but eh
        // much easier/better: select a random serialized index, then
        // inflateHexCoordinate
    while( isInvalidHexCoordinate( trollX, trollY )
           || smap.isDefault(trollX,trollY)
           || smap.get(trollX,trollY)
           || nonmonsterspawnRegion.contains( trollX, trollY ) ) {
        trollX = iprng() % 300;
        trollY = iprng() % 200;
    }
    using namespace std;
    if( trollLives ) {
        cerr << "troll spawned at " << trollX << ", " << trollY << endl;
    }

    // sources of classical music: http://en.wikipedia.org/wiki/Wikipedia:Sound/list
    sf::Music musicTest;
    musicTest.OpenFromFile( "./data-audio/test-background-music.ogg" );
    musicTest.Play();

    const int winWidth = 640, winHeight = 480;
    sf::RenderWindow win ( sf::VideoMode( winWidth ,winHeight,32), "TacClient demo" );
    HexViewport vp ( grid,  0, 0, winWidth, winHeight );

    win.SetFramerateLimit( 30 );
    while( win.IsOpened() ) {
        using namespace std;
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
                    if( !mx && !my ) break;
                    if( cmap.unitMayMoveTo( playerId, playerX + mx, playerY + my ) ) { // this check should be done both ss and cs
                        cmap.queueAction( new MovementAnimationCAction( cmap, playerId, mx, my ) );
                        cmap.queueAction( new NormalMovementCAction( cmap, playerId, mx, my ) );
                        playerX += mx;
                        playerY += my; // position immediately updated serverside
                        HexFovRegion newVision;
                        getVisionRegion( smap, playerX, playerY, newVision );
                        RevealTerrainCAction *rev = new RevealTerrainCAction( cmap );
                        SetActiveRegionCAction *act = new SetActiveRegionCAction( cmap );
                        // for now we just Send. Everything. wrt terrain revelations
                        for(HexFovRegion::const_iterator i = newVision.begin(); i != newVision.end(); i++) {
                            if( cmap.getTile( i->first, i->second ).getTileType() == 0 ) {
                                if( smap.get(i->first, i->second) ) {
                                    rev->add( i->first, i->second, &tileTypes["wall"] );
                                } else {
                                    rev->add( i->first, i->second, &tileTypes["floor"] );
                                }
                            }
                            act->add( i->first, i->second );
                        }
                        cmap.queueAction( rev );
                        cmap.queueAction( act );
                        // the troll never moves, so we never need to send UnitMove yet -- just UnitDiscover
                        // UnitDiscover has to send enough info to actually construct the unit!
                        // also to place it on the map, because otherwise it wouldn't have been
                        // discovered.
                        // on a real server even the initial setup would be done by events like these,
                        // so this is how your own units would get placed
                        if( !cmap.getTile(trollX,trollY).getActive() &&
                            newVision.contains( trollX, trollY ) &&
                            trollLives ) {
                            UnitDiscoverCAction *uds = new UnitDiscoverCAction( cmap, trollId, unitTypes["troll"], 1, 1, trollX, trollY, 0 );
                            cmap.queueAction( uds );
                        }
                    } else if( cmap.getTile( playerX + mx, playerY + my ).getUnitIdAt(0) != INVALID_ID ) {
                        int enemyUnitId = cmap.getTile( playerX + mx, playerY + my ).getUnitIdAt(0);

                        cmap.queueAction( new BumpAnimationCAction( cmap, playerId, mx, my ) );
                        cmap.queueAction( new PlaySoundCAction( cmap, soundBuffers["clink"].choose() ) );
                        cmap.queueAction( new RisingTextCAction( cmap, playerX + mx, playerY + my,
                                                                 cmap.getUnitById( enemyUnitId )->getUnitType().name + " was attacked!",
                                                                 200,200,200,
                                                                 512 ) );
                        cmap.queueAction( new BumpAnimationCAction( cmap, trollId, -mx, -my ) );
                        cmap.queueAction( new PlaySoundCAction( cmap, soundBuffers["clink"].choose() ) );
                        cmap.queueAction( new RisingTextCAction( cmap, playerX + mx, playerY + my,
                                                                 cmap.getUnitById( playerId )->getUnitType().name + " was attacked!",
                                                                 200,200,200,
                                                                 512 ) );
                    }
                }
                break;
            case sf::Event::Resized:
                using namespace std;
                vp.setRectangle(0, 0, win.GetWidth(), win.GetHeight());
                break;
            case sf::Event::Closed:
                win.Close();
                break;
        }
        double cx, cy;
        if( cmap.getUnitBaseScreenPositionById( playerId, cx, cy ) ) {
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

        glColor3f(1.0,1.0,1.0);

        vp.drawGL( cmap.getLevelBlitter(), win, win.GetWidth(), win.GetHeight() );
        vp.drawGL( cmap.getUnitBlitter(0), win, win.GetWidth(), win.GetHeight() );


        vp.beginClip( win.GetWidth(), win.GetHeight() );
        win.SetView(sf::View( sf::Vector2f(0,0), sf::Vector2f( ((double)win.GetWidth())/2.0, ((double)win.GetHeight())/2.0 ) ) );
        using namespace std;

        sf::View fxView( sf::Vector2f(0,0), sf::Vector2f( ((double)win.GetWidth())/2.0, ((double)win.GetHeight())/2.0 ) );
        vp.translateToHex( 0, 0, win.GetWidth(), win.GetHeight(), fxView );
        win.SetView( fxView );
        cmap.drawEffects( win, 0, 0 );
        win.SetView( win.GetDefaultView() );

        vp.endClip();

        win.Display();
    }
    
    return 0;
}
