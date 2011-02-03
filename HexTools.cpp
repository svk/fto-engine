#include "HexTools.h"

#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <stdexcept>

#include <iostream>

namespace HexTools {

int hexCircleSize(int r) {
    // 1 + 6 (1 + 2 + .. r)
    return 1 + 3 * r * (r+1);
}

int flattenHexCoordinate(int x, int y) {
    int i, j, r;
    polariseHexCoordinate( x, y, i, j, r );
    if( r == 0 ) return 0;
    return hexCircleSize(r-1) + i * r + j;
}

void inflateHexCoordinate(int i, int& x, int& y) {
    // Note -- this is not constant-time and should
    // only be used for testing. (Just store the IJR
    // or XY values!) For in-order iteration, it
    // should be comparatively simple to increment
    // IJR coordinates.
    if( i == 0 ) {
        x = y = 0;
        return;
    }
    assert( i > 0 );

    int r = 0;
    do {
        ++r;
    } while( hexCircleSize(r) <= i );
    int irj = i - hexCircleSize(r-1);
    int ri = irj / r;
    int rj = irj - ri * r;
    cartesianiseHexCoordinate(ri,rj,r,x,y);
}

void cartesianiseHexCoordinate(int i, int j, int r, int& x , int& y) {
    static const int dx[]={3,0,-3,-3,0,3},
                     dy[]={1,2,1,-1,-2,-1};
    const int ip = (i+1) % 6;
    x = dx[i] * (r-j) + dx[ip] * j;
    y = dy[i] * (r-j) + dy[ip] * j;
}

void polariseHexCoordinate(int x, int y, int& i, int& j, int& r) {
    if( (x%3) != 0 ) {
        using namespace std;
        cerr << "oi" << endl;
    }
    assert( (x%3) == 0 );
    x /= 3;
    if( x > 0 ) {
        if( y >= x ) {
            i = 0;
        } else if( y < -x ) {
            i = 4;
        } else {
            i = 5;
        }
    } else if( x < 0 ) {
        if( y > -x ) {
            i = 1;
        } else if( y <= x ) {
            i = 3;
        } else {
            i = 2;
        }
    } else {
        if( y > 0 ) {
            i = 1;
        } else if( y < 0 ) {
            i = 4;
        } else {
            i = j = r = 0;
            return;
        }
    }
    if( i == 1 || i == 4 ) {
        j = abs(x);
        r = abs(x) + (abs(y)-abs(x))/2;
    } else if( i == 0 || i == 3 ) {
        r = abs(x) + (abs(y)-abs(x))/2;
        j = r - abs(x);
    } else if( i == 2 ) {
        j = -(x+y) / 2;
        r = abs(x);
    } else {
        assert( i == 5 );
        j = (x+y) / 2;
        r = abs(x);
    }
}

void HexRegion::add(int x, int y) {
    coords.insert( HexCoordinate(x,y) );
}

void HexRegion::remove(int x,int y) {
    coords.erase( HexCoordinate(x,y) );
}

void HexRegion::clear(void) {
    coords.clear();
}

bool HexRegion::contains(int x, int y) const {
    std::set<HexCoordinate>::const_iterator i = coords.find( HexCoordinate(x,y) );
    return i != coords.end();
}

};
