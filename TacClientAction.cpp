#include "TacClientAction.h"

namespace Tac {

void RisingTextCAction::operator()(void) const {
    cmap.addRisingText( hexX, hexY, text, sf::Color(r,g,b), a );
}

void RevealTerrainCAction::operator()(void) const {
    for(RevelationsType::const_iterator i = revelations.begin(); i != revelations.end(); i++) {
        cmap.setTileType( i->first.first, i->first.second, i->second );
    }
    using namespace std;
    cerr << "revelations size: " << revelations.size() << endl;
}

void RemoveUnitCAction::operator()(void) const {
    cmap.removeUnit( unitId );
}

void UnitDiscoverCAction::operator()(void) const {
    cmap.adoptUnit( new ClientUnit( unitId, unitType, team, owner ) );
    cmap.placeUnitAt( unitId, x, y, layer );
}

void SetActiveRegionCAction::operator()(void) const {
    using namespace std;
    cerr << "active region size: " << region.size() << endl;

    cmap.updateActive( region );
}

void NormalMovementCAction::operator()(void) const {
    cmap.moveUnit( unitId, dx, dy );
}


void BumpAnimationCAction::operator()(void) const {
    ClientUnit *unit = cmap.getUnitById(unitId);
    if( unit ) {
        int x = dx, y = dy;
        cmap.getGrid().hexToScreen( x, y );
        unit->startMeleeAnimation( x, y );
        cmap.setAnimatedUnit( unit );
    }
}

void MovementAnimationCAction::operator()(void) const {
    ClientUnit *unit = cmap.getUnitById(unitId);
    if( unit ) {
        int x = dx, y = dy;
        cmap.getGrid().hexToScreen( x, y );
        unit->startMovementAnimation( x, y );
        cmap.setAnimatedUnit( unit );
    }
}


}
