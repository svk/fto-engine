#include "TacClient.h"

#include <cstdlib>
#include <cstdio>
#include <cassert>


namespace Tac {

const double MovementAnimationDuration = 0.15;

std::map< std::string, int > SpriteId::spritenoAliases;

ClientUnit::ClientUnit(int id, ClientUnitType& unitType, int team, int owner) :
    id ( id ),
    unitType ( unitType ),
    team ( team ),
    owner ( owner ),
    state ( LIVING ),
    x ( 0 ),
    y ( 0 ),
    layer ( 0 ),
    hasPosition ( false ),
    curveAnim ( 0 )
{
}

void ClientUnit::setLiving(bool val) {
    state = val ? LIVING : DEAD;
}

void ClientUnit::setPosition(int x_, int y_) {
    hasPosition = true;
    x = x_;
    y = y_;
}

void ClientUnit::setNoPosition(void) {
    hasPosition = false;
    x = y = 0;
}


int ClientUnit::getId(void) const {
    return id;
}

void ClientUnitManager::adopt(ClientUnit* unit) {
    ClientUnit *prev = (*this)[unit->getId()];
    // actually, there being a previous unit by this ID
    // is fine. just replace it (this is a simple way
    // of doing discover)
    if( prev ) {
        delete prev;
    }
    units[ unit->getId() ] = unit;
}

ClientUnitManager::~ClientUnitManager(void) {
    for(std::map<int,ClientUnit*>::iterator i = units.begin(); i != units.end(); i++) {
        delete i->second;
    }
    units.clear();
}

ClientUnit* ClientUnitManager::operator[](int id) const {
    std::map<int,ClientUnit*>::const_iterator i = units.find( id );
    if( i == units.end() ) return 0;
    return i->second;
}

ClientTile::ClientTile(void) :
    tileType ( 0 ),
    highlight ( NONE ),
    inactive ( true ),
    x ( 0 ),
    y ( 0 )
{
    clearUnits();
}

void ClientTile::setHighlight( ClientTile::Highlight highlight_ ) {
    highlight = highlight_;
}

void ClientTile::setActive(void) {
    inactive = false;
}

int ClientTile::clearUnitById(int id) {
    for(int i=0;i<UNIT_LAYERS;i++) if( unitId[i] == id ) {
        unitId[i] = 0;
        return i;
    }
    return -1;
}

void CMLevelBlitterGL::putSprite( const sf::Sprite& sprite ) {
    drawBoundSprite( sprite );
}

void ClientTile::clearUnits(void) {
    for(int i=0;i<UNIT_LAYERS;i++) {
        unitId[i] = INVALID_ID;
    }
}

void ClientTile::setInactive(void) {
    inactive = true;
    clearUnits();
}

void ClientMap::updateActive(const HexTools::HexRegion& visible) {
    if( !visible.contains( 0,0 ) ) {
        tiles.get(0,0).setInactive();
    } else {
        tiles.get(0,0).setActive();
    }
    for(int r=0;r<=radius;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++) {
        int x, y;
        HexTools::cartesianiseHexCoordinate( i, j, r, x, y );
        if( !visible.contains( x,y ) ) {
            tiles.get(x,y).setInactive();
        } else {
            tiles.get(x,y).setActive();
        }
    }
}

bool ClientUnit::getPosition(int& ox, int& oy) const {
    if( hasPosition ) {
        ox = x;
        oy = y;
    }
    return hasPosition;
}

void ClientMap::moveUnit(int id,int dx,int dy) {
    ClientUnit *unit = units[id];
    int lx, ly, layer;
    assert( unit );
    if( !unit->getPosition( lx, ly ) ) {
        throw std::logic_error( "unit cannot be moved without being placed" );
    }
    layer = unit->leaveTile( tiles.get( lx, ly ) );
    placeUnitAt( id, lx + dx, ly + dy, layer );
}

void ClientMap::placeUnitAt(int id,int x,int y,int layer) {
    ClientUnit *unit = units[id];
    int lx, ly;
    assert( unit );
    if( unit->getPosition( lx, ly ) ) {
        unit->leaveTile( tiles.get( lx, ly ) );
    }
    unit->enterTile( tiles.get( x, y ), layer );
}

void ClientUnit::enterTile(ClientTile& tile, int layer_) {
    tile.setUnit( id, layer_ );
    layer = layer_;
    setPosition( tile.getX(), tile.getY() );
}

int ClientUnit::leaveTile(ClientTile& tile) {
    return tile.clearUnitById( id );
}

void ClientTile::setUnit(int id, int layer) {
    assert( layer >= 0 && layer < UNIT_LAYERS );
    unitId[ layer ] = id;
}

ClientMap::ClientMap(int radius, TacSpritesheet& sheet, ScreenGrid& grid, FreetypeFace* risingTextFont) :
    risingTextFont ( risingTextFont ),
    radius ( radius ),
    tiles ( radius ),
    units (),
    animatedUnit ( 0 ),
    levelBlitter (*this, sheet),
    groundUnitBlitter (*this, sheet, 0),
    grid ( grid ),
    animRisingText ()
{
    for(int r=0;r<=radius;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++) {
        int x, y;
        HexTools::cartesianiseHexCoordinate( i, j, r, x, y );
        tiles.get(x,y).setXY(x,y);
    }
}

bool LineCurveAnimation::done(void) const {
    return phase >= duration;
}

void LineCurveAnimation::get(double& x, double& y) const {
    if( phase <= 0.0 ) {
        x = x0;
        y = y0;
    } else if( phase >= duration ) {
        x = x1;
        y = y1;
    } else {
        x = (x1-x0) * (phase/duration);
        y = (y1-y0) * (phase/duration);
    }
}

void ClientUnit::animate(double dt) {
    if( curveAnim ) {
        curveAnim->animate( dt );
    }
}

bool ClientUnit::done(void) const {
    if( curveAnim ) {
        return curveAnim->done();
    }
    return true;
}

void ClientUnit::completedAnimation(void) {
    delete curveAnim;
    curveAnim = 0;
}

void LineCurveAnimation::animate(double dt) {
    phase += dt;
}

LineCurveAnimation::LineCurveAnimation(double duration, double x0, double y0, double x1, double y1) :
    duration ( duration ),
    x0 ( x0 ),
    y0 ( y0 ),
    x1 ( x1 ),
    y1 ( y1 ),
    phase ( 0.0 )
{
}

void ClientMap::animate(double dt) {
    for(RTAManager::List::iterator i = animRisingText.objects.begin(); i != animRisingText.objects.end(); i++) {
        (*i)->animate( dt );
    }
    animRisingText.prune();
    if( animatedUnit ) {
        animatedUnit->animate( dt );
        if( animatedUnit->done() ) {
            animatedUnit->completedAnimation();
            animatedUnit = 0;
        }
    }
}

ClientActionQueue::~ClientActionQueue(void) {
    while( !actions.empty() ) {
        delete actions.front();
        actions.pop();
    }
}

bool ClientMap::shouldBlock(void) const {
    return isInBlockingAnimation();
}

bool ClientMap::isInBlockingAnimation(void) const {
    return animatedUnit != 0;
}

void ClientMap::adoptUnit(ClientUnit* unit) {
    units.adopt( unit );
}

void ClientMap::queueAction(ClientAction* action) {
    using namespace std;
    caq.adopt( action );
}

void ClientMap::processActions(void) {
    using namespace std;
    while( !shouldBlock() && !caq.empty() ) {
        ClientAction *action = caq.pop();
        (*action)();
    }
}

void ClientActionQueue::adopt(ClientAction* action) {
    actions.push( action );
}

ClientAction* ClientActionQueue::pop(void) {
    ClientAction* rv = actions.front();
    actions.pop();
    return rv;
}

bool ClientActionQueue::empty(void) const {
    return actions.empty();
}

void ClientUnit::getCenterOffset(int& ox, int& oy) const {
    if( curveAnim ) {
        double dox, doy;
        curveAnim->get( dox, doy );
        ox = (int)(0.5 + dox);
        oy = (int)(0.5 + doy);
    } else {
        ox = oy = 0;
    }
}

void ClientUnit::startMovementAnimation(int dx,int dy) {
    if( curveAnim ) {
        delete curveAnim;
    }
    curveAnim = new LineCurveAnimation( MovementAnimationDuration,
                                        0, 0,
                                        dx, dy );
}

const ClientUnit* ClientMap::getUnitById(int id) const {
    return units[id];
}

ClientUnit* ClientMap::getUnitById(int id) {
    return units[id];
}

void CMUnitBlitterGL::drawHex(int x, int y, sf::RenderWindow& win) {
    int xadjust, yadjust;
    int id = cmap.getTile(x,y).getUnitIdAt( layer );
    if( id == INVALID_ID ) return;
    ClientUnit *unit = cmap.getUnitById( id );
    if( !unit ) return; // !?
    const ClientUnitType& unitType = unit->getUnitType();
    unit->getCenterOffset( xadjust, yadjust );
    glTranslatef( xadjust, yadjust, 0 );
    drawBoundSpriteCentered( unitType.spriteNormal, cmap.getGrid().getHexWidth(), cmap.getGrid().getHexHeight() );
}

void CMLevelBlitterGL::drawHex(int x, int y, sf::RenderWindow& win) {
    const ClientTile& tile = cmap.getTile(x,y);
    bool isLit = tile.getActive();
    ClientTileType *tt = tile.getTileType();
    using namespace std;
    if( !tt ) return;
    if( tile.getHighlight() == ClientTile::NONE && isLit ) {
        putSprite( tt->spriteNormal );
    } else {
        putSprite( tt->spriteGrayscale );
    }
    if( !isLit ) {
        putSprite( spriteFogZone );
    } else switch( tile.getHighlight() ) {
        case ClientTile::NONE: break;
        case ClientTile::MOVE_ZONE:
            putSprite( spriteMoveZone );
            break;
        case ClientTile::ATTACK_ZONE:
            putSprite( spriteAttackZone );
            break;
    }
    putSprite( spriteThinGrid );
}

CMUnitBlitterGL& ClientMap::getUnitBlitter(int layer) {
    // note that the layers don't need to be drawn in
    // z order -- they're ordered "by importance".
    // meaning, 0 is the ground layer where most units
    // are.
    switch( layer ) {
        case 0: return groundUnitBlitter;
    }
    throw std::logic_error("no such layer");
}

bool ClientTileType::mayTraverse(const ClientUnitType& unitType) const {
    int disregardThat;
    return mayTraverse( unitType, disregardThat );
}

bool ClientTileType::mayTraverse(const ClientUnitType& unitType, int& outCost) const {
    if( border ) return false;
    if( mobility == Type::WALL ) return false;
    outCost = baseCost;
    return true;
}

ClientUnitType::ClientUnitType(TacSpritesheet& sheet, const std::string& alias, const std::string& name) :
    name ( name ),
    spriteNormal ( sheet.makeSprite( SpriteId( alias, SpriteId::NORMAL ) ) )
{
}

ClientTileType::ClientTileType(TacSpritesheet& sheet, const std::string& alias, const std::string& name, Type::Mobility mobility, Type::Opacity opacity, bool border, int baseCost ) :
    name ( name ),
    mobility ( mobility ),
    opacity ( opacity ),
    border ( border ),
    baseCost ( baseCost ),
    spriteNormal ( sheet.makeSprite( SpriteId( alias, SpriteId::NORMAL ) ) ),
    spriteGrayscale ( sheet.makeSprite( SpriteId( alias, SpriteId::GRAYSCALE ) ) )
{
}

void ClientMap::setTileType(int x, int y, ClientTileType* tt) {
    ClientTile& tile = tiles.get(x, y );
    if( &tile != &tiles.getDefault() ) {
        tiles.get(x,y).setTileType( tt );
    }
}

void ClientTile::setTileType(ClientTileType*tt) {
    if( tt->border ) {
        // borders are unknowable.
        tileType = 0;
    } else {
        tileType = tt;
    }
}

void ClientMap::clearHighlights(void) {
    for(HexRegion::const_iterator i = moveHighlightZone.begin(); i != moveHighlightZone.end(); i++) {
        tiles.get( i->first, i->second ).setHighlight( ClientTile::NONE );
    }
    for(HexRegion::const_iterator i = attackHighlightZone.begin(); i != attackHighlightZone.end(); i++) {
        tiles.get( i->first, i->second ).setHighlight( ClientTile::NONE );
    }
    moveHighlightZone.clear();
    attackHighlightZone.clear();
}

void ClientMap::addAttackHighlight(int x, int y) {
    tiles.get( x, y ).setHighlight( ClientTile::ATTACK_ZONE );
    attackHighlightZone.add( x, y );
}

void ClientMap::addMoveHighlight(int x, int y) {
    tiles.get( x, y ).setHighlight( ClientTile::MOVE_ZONE );
    moveHighlightZone.add( x, y );
}

bool ClientMap::isOpaque(int x,int y) const {
    ClientTileType *tt = tiles.get(x,y).getTileType();
    if( !tt ) return true;
    switch( tt->opacity ) {
        default:
        case Type::BLOCK:
            return true;
        case Type::CLEAR:
            return false;
    }
}

bool ClientMap::unitMayMoveTo(int id, int x, int y) const {
    const ClientUnit *unit = getUnitById( id );
    if( !unit ) return false;
    const ClientTile& tile = tiles.get( x, y );
    const ClientTileType* tileType = tile.getTileType();
    if( !tileType ) return false; // FOV should ensure that this is never possible anyway
                                  // exception for blinded units? probably best if even
                                  // they get r=1 vision, otherwise wonky
                                  // [an extra way to explore, by bumping and trying to
                                  //  move -- is this accessible without committing to
                                  //  a move? does it cost movement even if it fails?
                                  //  how is it indicated, how is the memory indicated?
                                  //  etc. -- unnecessary complications]
    return tileType->mayTraverse( unit->getUnitType() );
}

bool ClientMap::unitMayMove(int id, int dx, int dy) const {
    const ClientUnit *unit = getUnitById( id );
    if( !unit ) return false;
    int x, y;
    if( !unit->getPosition( x, y ) ) return false;
    return unitMayMoveTo( id, x + dx, y + dy);
}

bool ClientMap::getUnitScreenPositionById( int id, double& x, double& y ) const {
    const ClientUnit *unit = getUnitById( id );
    if( !unit ) return false;
    int hx, hy;
    if( !unit->getPosition( hx, hy ) ) return false;
    int ofx, ofy;
    unit->getCenterOffset( ofx, ofy );
    grid.hexToScreen( hx, hy );
    x = hx + ofx;
    y = hy + ofy;
    return true;
}

void ClientMap::addRisingText(int x,int y,const std::string& str, const sf::Color& col, int alpha) {
    if( risingTextFont ) {
        LabelSprite *label = new LabelSprite( str, col, *risingTextFont );
        grid.hexToScreen( x, y );
        x -= label->getWidth() / 2 - grid.getHexWidth()/2;
        y -= label->getHeight() / 2 - grid.getHexHeight()/2;
        animRisingText.adopt( new RisingTextAnimation( x, y, label, 1.5, 200.0, alpha ) );
    }
}

void ClientMap::drawEffects(sf::RenderWindow& win, double centerX, double centerY) {
    // assumes clipping and translation has been done
    for(RTAManager::List::iterator i = animRisingText.objects.begin(); i != animRisingText.objects.end(); i++) {
        using namespace std;
        win.Draw( **i );
    }
}


};
