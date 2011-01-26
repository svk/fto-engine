#include "Nash.h"

#include <stdexcept>

#include <cstdio>
#include <cstdlib>
#include <cassert>

#include <list>
#include <set>

namespace Nash {

bool NashTile::isColour(NashTile::Colour c) const {
    if( status == PIECE ) return c == colour;
    else if( status == EDGE ) switch( c ) {
        case WHITE: return colour == WHITE_BLACK || colour == WHITE || colour == BLACK_WHITE;
        case BLACK: return colour == WHITE_BLACK || colour == BLACK || colour == BLACK_WHITE;
        default: break;
    }
    return false;
}

NashBoard::NashBoard(int size) :
    size ( size ),
    tiles ( size )
{
    assert( (size%2) == 1 );
    const int border = (size+1)/2;

    for(int i=-border;i<=border;i++) for(int j=-border;j<=border;j++) {
        int x = i * 3, y = 2 * j + i;
        tiles.get(x,y).status = NashTile::FREE;
        tiles.get(x,y).colour = NashTile::NONE;
        if( abs(i) == border && abs(j) == border ) {
            continue;
        }
        if( abs(i) == border ) {
            tiles.get(x,y).status = NashTile::EDGE;
            tiles.get(x,y).colour = NashTile::WHITE;
        } else if( abs(j) == border ) {
            tiles.get(x,y).status = NashTile::EDGE;
            tiles.get(x,y).colour = NashTile::BLACK;
        }
    }
    tiles.get(-border*3,border).colour = NashTile::BLACK_WHITE;
    tiles.get(-border*3,border).status = NashTile::EDGE;
    tiles.get(border*3,-border).colour = NashTile::WHITE_BLACK;
    tiles.get(border*3,-border).status = NashTile::EDGE;
    tiles.getDefault().status = NashTile::OFF_MAP;
}

NashTile::Colour NashBoard::getWinner(void) const {
    const int border = (size+1)/2;
    const int dx[]= { 3, 0, -3, -3, 0, 3 };
    const int dy[]= { 1, 2, 1, -1, -2, -1 };
    const int bx = -border*3, by = border;
    const int ex = -bx, ey = -by;
    typedef std::pair<int,int> N;
    std::set< N > z;
    std::list< N > q;
    q.push_back( N( bx, by ) );
    while( !q.empty() ) {
        for(int i=0;i<6;i++) {
            int x = q.front().first + dx[i], y = q.front().second + dy[i];
            if( find( z.begin(), z.end(), N(x,y) ) != z.end() ) continue;
            if( tiles.get(x,y).isColour( NashTile::BLACK ) ) {
                q.push_back( N(x,y) );
                z.insert( N(x,y) );
            }
        }
        q.pop_front();
    }
    if( find( z.begin(), z.end(), N(ex, ey) ) != z.end() ) return NashTile::BLACK;
    q.clear();
    z.clear();
    q.push_back( N( bx, by ) );
    while( !q.empty() ) {
        for(int i=0;i<6;i++) {
            int x = q.front().first + dx[i], y = q.front().second + dy[i];
            if( find( z.begin(), z.end(), N(x,y) ) != z.end() ) continue;
            if( tiles.get(x,y).isColour( NashTile::WHITE ) ) {
                q.push_back( N(x,y) );
                z.insert( N(x,y) );
            }
        }
        q.pop_front();
    }
    if( find( z.begin(), z.end(), N(ex, ey) ) != z.end() ) return NashTile::WHITE;
    return NashTile::NONE;
}

NashBoard::~NashBoard(void) {
}

void NashBoard::put(int x,int y,NashTile::Colour c) {
    if( !isLegalMove(x,y) ) {
        throw std::runtime_error( "illegal move attempted" );
    }
    NashTile& tile = tiles.get(x,y);
    tile.status = NashTile::PIECE;
    tile.colour = c;
}

bool NashBoard::isLegalMove(int x,int y) const {
    const NashTile& tile = tiles.get(x,y);
    return tile.status == NashTile::FREE;
}

std::string NashBoard::getAppearance(int x, int y) const {
    const NashTile& tile = tiles.get(x,y);
    if( tile.status == NashTile::FREE ) {
        return "tile-free";
    } else if( tile.status == NashTile::PIECE ) {
        if( tile.colour == NashTile::WHITE ) {
            return "tile-white";
        } else if( tile.colour == NashTile::BLACK ) {
            return "tile-black";
        } else {
            return "";
        }
    } else if( tile.status == NashTile::EDGE ) {
        if( tile.colour == NashTile::WHITE ) {
            return "tile-edge-white";
        } else if( tile.colour == NashTile::BLACK ) {
            return "tile-edge-black";
        } else if( tile.colour == NashTile::WHITE_BLACK ) {
            return "tile-edge-white-black";
        } else if( tile.colour == NashTile::BLACK_WHITE ) {
            return "tile-edge-black-white";
        }
    }
    return "";
}

}
