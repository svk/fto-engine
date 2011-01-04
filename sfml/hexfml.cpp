#include "hexfml.h"

#include <cassert>
#include <stdexcept>

#include <iostream>

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

void ScreenGrid::screenToHex( int& x, int& y, int scrollX, int scrollY ) const {
    screenToGeometric( x, y, scrollX, scrollY );
    int col = getColumn( x );
    int row = getRow( col, y );
    cellClassify( x, y, col, row );
}

void ScreenGrid::hexToScreen( int& x, int& y ) const {
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

HexSprite::HexSprite(const std::string& filename, const ScreenGrid& grid) :
    grid ( grid ),
    image ( 0 ),
    sprite (),
    offsetX ( 0 ),
    offsetY ( 0 ),
    hx ( 0 ),
    hy ( 0 )
{
    image = new sf::Image();
    if( !image->LoadFromFile( filename ) ) {
        delete image;
        throw std::runtime_error( "failed to load image" );
    }
    image->SetSmooth( false );
    loadSpriteFrom( *image );
}

HexSprite::HexSprite(const sf::Image& externalImage, const ScreenGrid& grid) :
    grid ( grid ),
    image ( 0 ),
    sprite (),
    offsetX ( 0 ),
    offsetY ( 0 ),
    hx ( 0 ),
    hy ( 0 )
{
    loadSpriteFrom( externalImage );
}

void HexSprite::loadSpriteFrom(const sf::Image& img) {
    sprite.SetImage( img );
    offsetX = (grid.getHexWidth() - img.GetWidth()) / 2;
    offsetY = (grid.getHexHeight() - img.GetHeight()) / 2;
    setPosition( hx, hy );
}

void HexSprite::getPosition(int& hx_, int& hy_) const {
    hx_ = hx;
    hy_ = hy;
}

void HexSprite::setPosition(int hx_, int hy_) {
    int sx, sy;
    sx = hx = hx_;
    sy = hy = hy_;
    grid.hexToScreen( sx, sy );
    sprite.SetPosition( 0.5 + (double) (sx + offsetX),
                        0.5 + (double) (sy + offsetY) );
}

void HexSprite::draw(sf::RenderWindow& win) const {
    win.Draw( sprite );
}

HexSprite::~HexSprite(void) {
    if( image ) {
        delete image;
    }
}

HexViewport::HexViewport(const ScreenGrid& grid, int x0, int y0, int w, int h) :
    grid (grid),
    screenXOffset (x0),
    screenYOffset (y0),
    screenWidth (w),
    screenHeight (h),
    centerX (0),
    centerY (0)
{
}

void HexViewport::center(int cx, int cy) {
    centerX = cx;
    centerY = cy;
}

void HexViewport::setRectangle(int x0, int y0, int w, int h) {
    screenXOffset = x0;
    screenYOffset = y0;
    screenWidth = w;
    screenHeight = h;
}

void HexViewport::draw(HexBlitter& blitter, sf::RenderWindow& win, sf::View& view) const {
    using namespace std;

    int hxw = grid.getHexWidth(),
        hxh = grid.getHexHeight();
    // may want to add padding to hxw,hxh to draw larger-than-hex sprites
    // correctly

    int hwx0 = centerX - screenWidth/2,
        hwy0 = centerY - screenHeight/2;
    int hwx1 = hwx0 + screenWidth - 1,
        hwy1 = hwy0 + screenHeight - 1;

    hwx0 = screenXOffset;
    hwy0 = screenYOffset; 
    hwx1 = screenXOffset + screenWidth - 1;
    hwy1 = screenYOffset + screenHeight - 1; 
    translateCoordinates( hwx0, hwy0 );
    translateCoordinates( hwx1, hwy1 );

    glScissor( screenXOffset, win.GetHeight() - (screenYOffset + screenHeight), screenWidth, screenHeight );
    glEnable( GL_SCISSOR_TEST );

    win.Clear( sf::Color(255,255,255) );

    grid.screenToHex( hwx0, hwy0, 0, 0 );
    grid.screenToHex( hwx1, hwy1, 0, 0 );

    hwx0 /= 3;
    hwx1 /= 3;
    hwx0--;
    hwy0++;
    hwx1++;
    hwy1--;

    int rsw = win.GetWidth(), rsh = win.GetHeight();
    int htw = grid.getHexWidth()/2, hth = grid.getHexHeight()/2;
    int hw = screenWidth/2, hh = screenHeight/2;
    for(int i=hwx0;i<=hwx1;i++) {
        for(int j=hwy1;j<=hwy0;j++) {
            if( ((i%2)!=0) != ((j%2)!=0) ) continue;
            int sx = 3 * i, sy = j;
            grid.hexToScreen( sx, sy );
            view.SetCenter( -((double) sx - centerX + screenXOffset - hw), // add to move left
                            -((double) sy - centerY + screenYOffset - hh)); // add to move up
            view.SetCenter( (rsw/2) - screenXOffset - hw + htw - sx + centerX,
                            (rsh/2) - screenYOffset - hh + hth - sy + centerY);
            blitter.drawHex( 3 * i, j, win );
        }
    }

    glDisable( GL_SCISSOR_TEST );
}

void ScreenGrid::centerRectangle(sf::FloatRect& rect) {
    int hoffset = (width - rect.GetWidth())/2,
        voffset = (height - rect.GetHeight())/2;
    // Note -- the use of integers here is NOT an error.
    // We don't want to suddenly put blurry half-pixels just
    // because we're centering oddly.
    rect.Offset( hoffset, voffset );
}

bool HexViewport::translateCoordinates( int& x, int& y ) const {
    int mx = x - screenXOffset, my = y - screenYOffset;
    using namespace std;
    if( mx < 0 || my < 0 || mx >= screenWidth || my >= screenHeight ) {
        return false;
    }
    x = mx + centerX - (screenWidth/2) + grid.getHexWidth()/2;
    y = my + centerY - (screenHeight/2) + grid.getHexHeight()/2;
    return true;
}

ViewportMouseScroller::ViewportMouseScroller( HexViewport& vp, const sf::Input& input ) :
    vp ( vp ),
    input ( input ),
    mouseX0 ( input.GetMouseX() ),
    mouseY0 ( input.GetMouseY() ),
    vpX0 ( vp.getCenterX() ),
    vpY0 ( vp.getCenterY() )
{
}

void ViewportMouseScroller::scroll(void) {
    const int dx = input.GetMouseX() - mouseX0,
              dy = input.GetMouseY() - mouseY0;
    const int multiplier = 2;
    using namespace std;
    vp.center( vpX0 + multiplier * dx, vpY0 + multiplier * dy );
}
