#include "hexfml.h"

#include <cassert>
#include <stdexcept>

#include <stdio.h>

int ScreenGrid::getColumn(int gx) const {
    if( gx >= 0 ) {
        return gx / hOffsetIncrement;
    } else {
        return (gx - (hOffsetIncrement-1)) / hOffsetIncrement;
    }
}

void ScreenGrid::screenToGeometric( int& x, int& y, int scrollX, int scrollY ) const {
    x += scrollX;
    y += scrollY;
    y = -y;
}

void ScreenGrid::geometricToScreen( int& x, int& y, int scrollX, int scrollY ) const {
    y = -y;
    x -= scrollX;
    y -= scrollY;
}

int ScreenGrid::getRow( int col, int gy ) const {
    const int h = 2 * vOffsetIncrement;
    if( col % 2 != 0 ) {
        gy += vOffsetIncrement;
    }
    if( gy >= 0 ) {
        return gy / h;
    } else {
        return (gy - (h-1))/h;
    }
}

void ScreenGrid::cellClassify( int& gx, int& gy, int col, int row ) const {
    const int cx = 3 * col,
              cy = 2 * (row+1) - ((col % 2) ? 1 : 0);
    uint32_t sample;
    gx -= col * hOffsetIncrement;
    gy -= row * 2 * vOffsetIncrement;
    if( col % 2 ) {
        gy += vOffsetIncrement;
    }
    gy = 2 * vOffsetIncrement - 1 - gy;
    assert( gx >= 0 && gx < width );
    assert( gy >= 0 && gy < height );
    sample = pixels[gx + gy * width];
    if( sample == mainColour ) {
        gx = cx;
        gy = cy;
    } else if( sample == northwestColour ) {
        gx = cx - 3;
        gy = cy + 1;
    } else if( sample == northeastColour ) {
        gx = cx + 3;
        gy = cy - 1;
    } else if( sample == southwestColour ) {
        gx = cx - 3;
        gy = cy - 1;
    } else {
        assert( sample == southeastColour );
        gx = cx + 3;
        gy = cy - 1;
    }
}

void ScreenGrid::analysePrototype(void) {
    width = hexPrototype.GetWidth();
    height = hexPrototype.GetHeight();

    if( height % 2 != 0 ) {
        throw std::runtime_error( "prototype must be of even height" );
    }

    vOffsetIncrement = height / 2;

    pixels = reinterpret_cast<const uint32_t*>( hexPrototype.GetPixelsPtr() );
    northwestColour = pixels[0];
    northeastColour = pixels[width-1];
    southwestColour = pixels[(height-1)*width];
    southeastColour = pixels[(width-1)+(height-1)*width];

    int nFound = 0;
    for(int i=0;i<(width*height);i++) {
        if( pixels[i] == northwestColour ) continue;
        if( pixels[i] == southwestColour ) continue;
        if( pixels[i] == southeastColour ) continue;
        if( pixels[i] == northeastColour ) continue;
        if( nFound > 0 && mainColour != pixels[i] ) {
            throw std::runtime_error( "prototype must have unique main colour" );
        }
        ++nFound;
        mainColour = pixels[i];
    }

    for(hOffsetIncrement=0;hOffsetIncrement<width;hOffsetIncrement++) {
        if(pixels[hOffsetIncrement] == northeastColour) break;
    }
    if( hOffsetIncrement >= width ) {
        throw std::runtime_error( "unable to calculate horizontal offset increment from prototype" );
    }
}

void ScreenGrid::screenToHex( int& x, int& y, int scrollX, int scrollY ) {
    screenToGeometric( x, y, scrollX, scrollY );
    int col = getColumn( x );
    int row = getRow( col, y );
    cellClassify( x, y, col, row );
}

void ScreenGrid::hexToScreen( int& x, int& y ) {
    // (0,0) -> (0,0)
    // (3,-1) -> (hOffsetIncrement,vOffsetIncrement)
    assert( (x%3) == 0 );
    x = (x/3) * hOffsetIncrement;
    y = (-y) * vOffsetIncrement;
}

ScreenGrid::ScreenGrid( const std::string& prototypeFilename ) {
    if( !hexPrototype.LoadFromFile( prototypeFilename ) ) {
        throw std::runtime_error( "unable to load prototype file" );
    }

    analysePrototype();
}

ScreenGrid::~ScreenGrid(void) {
}

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
