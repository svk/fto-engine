#include "TacClientAction.h"

namespace Tac {

void RevealTerrainCAction::operator()(void) const {
    for(RevelationsType::const_iterator i = revelations.begin(); i != revelations.end(); i++) {
        cmap.setTileType( i->first.first, i->first.second, i->second );
    }
}

void UnitDiscoverCAction::operator()(void) const {
    cmap.adoptUnit( new ClientUnit( unitId, unitType, team, owner ) );
    cmap.placeUnitAt( unitId, x, y, layer );
}

void SetActiveRegionCAction::operator()(void) const {
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
        unit->startMovementAnimation( x, y );
        cmap.setAnimatedUnit( unit );
    }
}


}
