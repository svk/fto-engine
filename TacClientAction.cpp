#include "TacClientAction.h"

#include <sstream>

namespace Tac {

void DarkenCAction::operator()(void) const {
    for(std::vector<std::pair<int,int> >::const_iterator i = darkenTiles.begin(); i != darkenTiles.end(); i++) {
        cmap.darkenTile( i->first, i->second );
    }
}

void BrightenCAction::operator()(void) const {
    for(std::list<BrightenTile>::const_iterator i = brightenTiles.begin(); i != brightenTiles.end(); i++) {
        cmap.brightenTile( i->x, i->y, i->tt );
    }
}

void ApplyAttackResultSubstantialCAction::operator()(void) const {
    ClientUnit *aunit = cmap.getUnitById( attackerId );
    using namespace std;
    cerr << "pot move en pre: " << aunit->getAP().getPotentialMovementEnergy() << endl;
    aunit->getAP().spendActionPoint( 1 );
    cerr << "pot move en post: " << aunit->getAP().getPotentialMovementEnergy() << endl;
    ClientUnit *dunit = cmap.getUnitById( defenderId );
    if( !dunit ) return;
    if( result.status == AttackResult::HIT ) {
        dunit->setHp( dunit->getHp() - result.damage );
    }
}

void ApplyAttackResultCosmeticCAction::operator()(void) const {
    ClientUnit *dunit = cmap.getUnitById( defenderId );
    int defx, defy;
    if( !dunit ) return;
    if( !dunit->getPosition( defx, defy ) ) return;
    if( result.status == AttackResult::MISS ) {
        cmap.addRisingText( defx, defy, "*miss*", sf::Color(80, 80, 80), 500 );
    } else {
        std::ostringstream oss;
        oss << "*-" << result.damage << "hp!*";
        cmap.playSound( cmap.getSound( "clink" ) );
        cmap.addRisingText( defx, defy, oss.str(), sf::Color(80, 80, 80), 500 );
    }
}

void PlaySoundCAction::operator()(void) const {
    cmap.playSound( &buffer );
}

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

void BeginPlayerTurnCAction::operator()(void) const {
    cmap.playerTurnBegins( playerId );
}

void RemoveUnitCAction::operator()(void) const {
    cmap.removeUnit( unitId );
}

void UnitDiscoverCAction::operator()(void) const {
    using namespace std;
    ClientUnit *unit = cmap.createUnit( unitId, unitType, team, owner, hp, maxHp );
    cerr << "creating unit w owner " << owner << endl;
    unit->getAP() = ap;
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
