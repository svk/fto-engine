#include "HtGo.h"

#include <cstdio>
#include <stdexcept>
#include <iostream>

HexTorusGoMap::HexTorusGoMap(int radius) :
    radius ( radius ),
    coreMap ( radius )
{
    int i, j, r;
    for(r=1;r<=radius;r++) {
        for(i=0;i<6;i++) {
            for(j=0;j<r;j++) {
                int x, y;
                cartesianiseHexCoordinate( i, j, r, x, y );
                coreMap.get(x,y).coreX = x;
                coreMap.get(x,y).coreY = y;
            }
        }
    }
}

void HexTorusGoMap::debugLabelCore(void) {
    char buffer[1024];
    int i, j, r, n = 0;
    coreMap.get(0,0).positionalLabel = "0";
    for(r=1;r<radius;r++) {
        for(i=0;i<6;i++) {
            for(j=0;j<r;j++) {
                int x, y;
                cartesianiseHexCoordinate( i, j, r, x, y );
                sprintf(buffer, "%d", ++n );
                coreMap.get(x,y).positionalLabel = std::string( buffer );
            }
        }
    }
    n = 0;
    r = radius;
    for(i=0;i<6;i++) {
        for(j=0;j<r;j++) {
            int x, y;
            cartesianiseHexCoordinate( i, j, r, x, y );
            using namespace std;
            sprintf(buffer, "R%d", ++n );
            coreMap.get(x,y).positionalLabel = std::string( buffer );
        }
    }
    coreMap.getDefault().positionalLabel = "ERR";
}

HtGoTile& HexTorusGoMap::getIJR(int i,int j,int r) {
    int x, y;
    cartesianiseHexCoordinate( i, j, r, x, y );
    return get( x, y );
}

HtGoTile& HexTorusGoMap::get(int x, int y) {
    int guard = 100;
    const int ox = x, oy = y;
    int i, j, r;
    polariseHexCoordinate( x, y, i, j, r );
    if( r < radius ||
        ( (r == radius) &&
          ( (i == 0 && j != 0) ||
            (i == 1) ||
            (i == 2) ) ) ) {
        return coreMap.get( x, y );
    }
    if( r == radius ) switch( i ) {
        case 0:
        case 5:
            return get( -x, y );
        case 3:
            return getIJR( 0, radius - j, r );
        case 4:
            return getIJR( 1, radius - j, r );
        default:
            throw std::logic_error( "illegal late-return state on rim of core hex" );
    }
    using namespace std;
    y %= 6 * radius;
    x %= 6 * radius;
    x -= 6 * radius;
    y -= 6 * radius;
    do {
        polariseHexCoordinate( x, y, i, j, r );
        if( r <= radius ) {
            return get(x,y);
        }
        if( y > 0 ) {
            if( x > 0 ) {
                x -= 3 * radius;
                y -= 3 * radius;
            } else {
                x += 3 * radius;
                y -= 3 * radius;
            }
        } else {
            if( x > 0 ) {
                x -= 3 * radius;
                y += 3 * radius;
            } else {
                x += 3 * radius;
                y += 3 * radius;
            }
        }
        if( --guard <= 0 ) {
            throw std::logic_error( "illegal state - probably looping infinitely");
        }
    } while(true);
}
