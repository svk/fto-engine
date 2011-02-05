#include "mtrand.h"

#include "TacClient.h"

int main(int argc, char *argv[]) {
    using namespace HexTools;
    using namespace Tac;

    const int mapSize = 40;

    SpriteId::bindAlias( "unit-smiley", 0 );
    SpriteId::bindAlias( "unit-troll", 1 );
    SpriteId::bindAlias( "tile-floor", 2 );
    SpriteId::bindAlias( "tile-wall", 3 );
    SpriteId::bindAlias( "zone-fog", 4 );
    SpriteId::bindAlias( "zone-move", 5 );
    SpriteId::bindAlias( "zone-attack", 6 );
    SpriteId::bindAlias( "thin-grid", 7 );

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
    sheet.adopt( SpriteId("zone-red", SpriteId::NORMAL),
                 grid.createSingleColouredImage( sf::Color( 255,0,0,128 ) ) );
    sheet.adopt( SpriteId("zone-green", SpriteId::NORMAL),
                 grid.createSingleColouredImage( sf::Color( 0,255,0,128 ) ) );
    sheet.adopt( SpriteId("thin-grid", SpriteId::NORMAL),
                 loadImageFromFile( "./data/hexthingrid2.png" ) );

    ResourceManager<ClientTileType> tileTypes;
    ResourceManager<ClientUnitType> unitTypes;

    unitTypes.bind( "player", new ClientUnitType( sheet, "unit-smiley", "Player" ) );

    tileTypes.bind( "border", new ClientTileType( sheet, "tile-wall", "hard wall", Type::WALL, Type::BLOCK, true, 0 ) );
    tileTypes.bind( "floor", new ClientTileType( sheet, "tile-floor", "floor", Type::FLOOR, Type::CLEAR, false, 100 ) );
    tileTypes.bind( "wall", new ClientTileType( sheet, "tile-wall", "wall", Type::WALL, Type::BLOCK, false, 0 ) );

    const int playerId = 1;
    const int playerTeam = 0, playerNo = 0;

    ClientMap cmap ( mapSize, sheet );
    cmap.adoptUnit( new ClientUnit( playerId, unitTypes["player"], playerTeam, playerNo ) );
    cmap.placeUnitAt( playerId, 0, 0, 0 );

    MTRand prng ( 1337 );
    HexMap<bool> smap ( mapSize );
    for(int r=1;r<mapSize;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++) {
        int x, y;
        cartesianiseHexCoordinate( i, j, r, x, y );
        smap.get(x,y) = prng() > 0.5;
    }
    
    return 0;
}
