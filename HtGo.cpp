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
    coreMap.get(0,0).coreX = 0;
    coreMap.get(0,0).coreY = 0;
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

void PointSet::add(const HtGoTile& tile) {
    ips.insert( std::pair<int,int>( tile.coreX, tile.coreY ) );
}

void PointSet::remove(const HtGoTile& tile) {
    InternalPointSet::iterator i = ips.find( std::pair<int,int>( tile.coreX, tile.coreY ) );
    if( i != ips.end() ) {
        ips.erase( i );
    }
}

bool PointSet::has(const HtGoTile& tile) const {
    InternalPointSet::const_iterator i = ips.find( std::pair<int,int>( tile.coreX, tile.coreY ) );
    return i != ips.end();
}

HtGoTile& HexTorusGoMap::getNeighbourOf(int x,int y,int i) {
    const static int dx[] = { 3, 0, -3, -3, 0, 3 },
                     dy[] = { 1, 2, 1, -1, -2, -1 };
    return get(x + dx[i%6], y + dy[i%6]);
}

HtGoTile& HexTorusGoMap::get(std::pair<int,int> xy) {
    return get( xy.first, xy.second );
}

HtGoTile& HexTorusGoMap::getNeighbourOf(const HtGoTile& tile, int i) {
    return getNeighbourOf(tile.coreX, tile.coreY, i );
}

PointSet HexTorusGoMap::libertiesOf(const PointSet& group) {
    PointSet rv;
    for(PointSet::const_iterator i = group.begin(); i != group.end(); i++) {
        const HtGoTile& tile = get( *i );
        for(int i=0;i<6;i++) {
            HtGoTile& nb = getNeighbourOf( tile, i );
            if( nb.state == HtGoTile::BLANK ) {
                rv.add( nb );
            }
        }
    }
    return rv;
}

PointSet HexTorusGoMap::groupOf(int x, int y) {
    using namespace std;
    HtGoTile& tile = get(x,y);
    return groupOf( tile );
}

PointSet HexTorusGoMap::groupOf(const HtGoTile& tile) {
    using namespace std;
    PointSet rv, open;
    open.add( tile );
    rv.add( tile );
    while( !open.empty() ) {
        std::pair<int,int> coords = open.pop();
        HtGoTile& tile = get( coords );

        for(int i=0;i<6;i++) {
            HtGoTile& nb = getNeighbourOf( tile, i );
            if( nb.state == tile.state ) {
                if( !rv.has( nb ) ) {
                    open.add( nb );
                    rv.add( nb );
                }
            }
        }
    }
    return rv;
}

std::pair<int,int> PointSet::pop(void) {
    InternalPointSet::iterator i = ips.begin();
    if( i == ips.end() ) {
        throw std::runtime_error( "popping from empty set" );
    }
    std::pair<int,int> rv = *i;
    ips.erase( i );
    return rv;
}

int HexTorusGoMap::removeGroup(const PointSet& xys) {
    int rv = xys.size();
    for(PointSet::const_iterator i = xys.begin(); i != xys.end(); i++) {
        get( i->first, i->second ).state = HtGoTile::BLANK;
    }
    return rv;
}

int HexTorusGoMap::put(int x,int y, HtGoTile::TileState st) {
    using namespace std;
    int enemyCaptures = 0, selfCaptures = 0;
    int singleCaptureX, singleCaptureY;
    get(x,y).state = st;
    for(int i=0;i<6;i++) {
        HtGoTile& tile = getNeighbourOf( x, y, i );
        if( tile.state == st ) continue;
        if( tile.state == HtGoTile::BLANK ) continue;
        PointSet group = groupOf( tile );
        PointSet libs = libertiesOf( group );
        if( libs.empty() ) {
            singleCaptureX = tile.coreX;
            singleCaptureY = tile.coreY;
            enemyCaptures += removeGroup( group );
        }
    }
    PointSet selfGroup = groupOf( x, y );
    if( libertiesOf( selfGroup ).empty() ) {
        selfCaptures += removeGroup( selfGroup );
    }
    if( enemyCaptures == 1 && selfCaptures == 0 ) {
        koActive = true;
        koCoreX = singleCaptureX;
        koCoreY = singleCaptureY;
    } else {
        koActive = false;
    }
    return enemyCaptures;
}

HexTorusGoMap::HexTorusGoMap(const HexTorusGoMap& that) :
    radius ( that.radius ),
    coreMap ( that.coreMap ),
    koActive ( that.koActive ),
    koCoreX ( that.koCoreX ),
    koCoreY ( that.koCoreY )
{
}

const HexTorusGoMap& HexTorusGoMap::operator=(const HexTorusGoMap& that) {
    if( this != &that ) {
        radius = that.radius;
        coreMap = that.coreMap;
        koActive = that.koActive;
        koCoreX = that.koCoreX;
        koCoreY = that.koCoreY;
    }
    return *this;
}

bool HexTorusGoMap::putWouldBeLegal(int x,int y, HtGoTile::TileState st) {
    // TODO constify hax
    if( st == HtGoTile::BLANK ) return false;

    HtGoTile& tile = get(x,y);

    if( tile.state != HtGoTile::BLANK ) return false;

    if( koActive && koCoreX == tile.coreX && koCoreY == tile.coreY ) {
        return false;
    }
    
    HexTorusGoMap copy = *this;
    copy.put( x, y, st );
    if( copy.get(x,y).state != st ) return false; // suicide is illegal
    
    return true;
}
