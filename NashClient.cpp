#include "NashClient.h"

namespace Nash {

void NashBlitter::drawHex(int x, int y, sf::RenderWindow& win) {
    std::string app = board.getAppearance(x,y);
    if( app.length() > 0 ) {
        hsman[ app ].draw( win );
    }
    if( selected && x == selectedX && y == selectedY ) {
        hsman[ "border-selection" ].draw( win );
    }
}

void NashBlitter::setSelected(int x,int y) {
    selected = true;
    selectedX = x;
    selectedY = y;
}

void NashBlitter::setNoSelected(void) {
    selected = false;
}

}
