#include "TacClient.h"

#include <cstdlib>
#include <cstdio>
#include <cassert>


namespace Tac {

const double MovementAnimationDuration = 0.15;

ClientUnit::ClientUnit(int id, ClientUnitType& unitType, int team, int owner) :
    id ( id ),
    unitType ( unitType ),
    team ( team ),
    owner ( owner ),
    state ( LIVING ),
    layer ( 0 ),
    x ( 0 ),
    y ( 0 ),
    hasPosition ( false )
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
    assert( !prev );
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
    const float width = sprite.GetSize().x, height = sprite.GetSize().y;
    const sf::FloatRect rect = sprite.GetImage()->GetTexCoords( sprite.GetSubRect() );
    glBegin( GL_QUADS );
    glTexCoord2f( rect.Left, rect.Top ); glVertex2f(0+0.5,0+0.5);
    glTexCoord2f( rect.Left, rect.Bottom ); glVertex2f(0+0.5,height+0.5);
    glTexCoord2f( rect.Right, rect.Bottom ); glVertex2f(width+0.5,height+0.5);
    glTexCoord2f( rect.Right, rect.Top ); glVertex2f(width+0.5,0+0.5);
    glEnd();
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
    for(int r=0;r<radius;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++) {
        int x, y;
        HexTools::cartesianiseHexCoordinate( i, j, r, x, y );
        if( !visible.contains( x,y ) ) {
            tiles.get(x,y).setInactive();
        } else {
            tiles.get(x,y).setActive();
        }
    }
}

bool ClientUnit::getPosition(int& ox, int& oy) {
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
    x = tile.getX();
    y = tile.getY();
}

int ClientUnit::leaveTile(ClientTile& tile) {
    return tile.clearUnitById( id );
}

void ClientTile::setUnit(int id, int layer) {
    assert( layer >= 0 && layer < UNIT_LAYERS );
    unitId[ layer ] = id;
}

ClientMap::ClientMap(int radius) :
    radius ( radius ),
    tiles ( radius ),
    units (),
    animatedUnit ( 0 )
{
    for(int r=0;r<radius;r++) for(int i=0;i<6;i++) for(int j=0;j<r;j++) {
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
    } else if( phase >= 1.0 ) {
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

void ClientMap::queueAction(const ClientAction& action) {
    caq.adopt( action.duplicate() );
}

void ClientMap::processActions(void) {
    while( !shouldBlock() && !caq.empty() ) {
        ClientAction *action = caq.pop();
        (*action)();
    }
}

bool ClientActionQueue::empty(void) const {
    return actions.empty();
}

void ClientUnit::getCenterOffset(double& ox, double& oy) const {
    if( curveAnim ) {
        curveAnim->get( ox, oy );
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

ClientUnit* ClientMap::getUnitById(int id) {
    return units[id];
}

void MovementAnimationCAction::operator()(void) const {
    ClientUnit *unit = cmap.getUnitById(unitId);
    if( unit ) {
        unit->startMovementAnimation( dx, dy );
    }
}

void CMLevelBlitterGL::drawHex(int x, int y, sf::RenderWindow& win) {
    ClientTileType *tt = cmap.tiles.get(x,y).getTileType();
    if( !tt ) return;
    putSprite( tilesheet.makeSprite( tt->spriteno ) );
}

};