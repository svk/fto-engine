#include "HexFov.h"

#include <algorithm>
#include <stdexcept>

#include <iostream>

namespace HexTools {

void HexFovNorthBeam::passFrom(int x, int y, const Angle& begin, const Angle& end) {
    const bool passEast = !(x == y),
               passWest = !(x == -y);
    Angle t0 ( x + 2, y ),
          t1 ( x + 1, y + 1 ),
          t2 ( x - 1, y + 1 ),
          t3 ( x - 2, y );
    Angle a, b;
    using namespace std;
    if( passEast && sectorIntersection( begin, end, t0, t1, a, b ) ) {
        primary->add( x + 3, y + 1, a, b );
    }
    if( sectorIntersection( begin, end, t1, t2, a, b ) ) {
        secondary->add( x, y + 2, a, b );
    }
    if( passWest && sectorIntersection( begin, end, t2, t3, a, b ) ) {
        primary->add( x - 3, y + 1, a, b );
    }
}

void HexFovNorthBeam::calculate(void) {
    int x, y;
    Angle begin, end;
    while( popNext(x, y, begin, end) ) {
        setLighted( x, y );
        if( !isOpaque( x, y ) ) {
            using namespace std;
            passFrom( x, y, begin, end );
        }
    }
}

bool LightedTileQueue::empty(void) const {
    return q.empty();
}

void LightedTileQueue::popFront(void) {
    q.erase( q.begin() );
}

void LightedTileQueue::add(int x, int y, const Angle& begin,const Angle& end) {
    using namespace std;
    std::pair<int,int> xy(x,y);
    LtqMap::iterator i = q.find( xy );
    if( i == q.end() ) {
        q[xy] = std::pair<Angle,Angle>( begin, end );
    } else {
        Angle nb, ne;
        sectorAdjacentUnion( begin, end, i->second.first, i->second.second, nb, ne );
        q[xy] = std::pair<Angle,Angle>( nb, ne );
    }
}

void LightedTileQueue::getFront(int& x_, int& y_, Angle& begin_, Angle& end_) {
    x_ = q.begin()->first.first;
    y_ = q.begin()->first.second;
    begin_ = q.begin()->second.first;
    end_ = q.begin()->second.second;
}

bool HexFovNorthBeam::popNext(int& x, int& y, Angle& begin, Angle& end) {
    if( !current->empty() ) {
        current->getFront( x, y, begin, end );
        current->popFront();
        return true;
    }
    if( primary->empty() && secondary->empty() ) {
        return false;
    }
    LightedTileQueue *t = current;
    current = primary;
    primary = secondary;
    secondary = t;
    return popNext( x, y, begin, end );
}

bool HexFovNorthBeam::isOpaque(int dx, int dy) const {
    return map.isOpaque( cx + dx, cy + dy );
}

Angle::Angle(void) :
    x ( 1 ),
    y ( 0 )
{
}

Angle::Angle(int x, int y) :
    x ( x ),
    y ( y )
{
}

int Angle::quadrant(void) const {
    if( x >= 0 ) {
        if( y >= 0 ) return 0;
        else return 3;
    } else {
        if ( y >= 0 ) return 1;
        else return 2;
    }
}

int Angle::compare(const Angle& that) const {
    const int q = quadrant();
    const int tq = that.quadrant();
    if( q > tq ) return 1;
    if( tq > q ) return -1;
    if( y && !x ) {
        if( that.y && !that.x ) {
            return 0;
        }
        return -1;
    }
    if( that.y && !that.x ) {
        return 1;
    }
    return y * that.x - x * that.y;
}

bool Angle::operator>(const Angle& that) const {
    return compare( that ) > 0;
}

bool Angle::operator<(const Angle& that) const {
    return compare( that ) < 0;
}

bool Angle::operator>=(const Angle& that) const {
    return compare( that ) >= 0;
}

bool Angle::operator<=(const Angle& that) const {
    return compare( that ) <= 0;
}

bool Angle::operator==(const Angle& that) const {
    return compare( that ) == 0;
}

Angle::Angle(const Angle& that) :
    x ( that.x ),
    y ( that.y )
{
}

const Angle& Angle::operator=(const Angle& that) {
    if( this != &that ) {
        x = that.x;
        y = that.y;
    }
    return *this;
}

void Sector::adjacentUnion(const Sector& that) {
    using namespace std;
    if( begin == that.end ) {
        begin = that.begin;
    } else if( end == that.begin ) {
        end = that.end;
    }
}

bool Sector::contains(const Angle& a) const {
    if( empty ) return false;
    if( containsBranchCut ) {
        return (a >= begin) || (a <= end);
    } else {
        return (a >= begin) && (a <= end);
    }
}

bool Sector::intersect(const Sector& that) {
    if( empty || that.empty ) {
        empty = true;
        return false;
    }
    bool bcontained = contains( that.begin ),
         econtained = contains( that.end );
    if( bcontained || econtained ) {
        if( bcontained ) {
            begin = that.begin;
        }
        if( econtained ) {
            end = that.end;
        }
        recheckBranchCut();
        return true;
    }
    if( !that.contains( begin ) ) {
        empty = true;
        return false;
    }
    return true;
}

bool sectorIntersection( const Angle& a0, const Angle& a1,
                         const Angle& b0, const Angle& b1,
                         Angle& r0, Angle& r1) {
    Sector a (a0, a1), b (b0, b1);
    a.intersect( b );
    r0 = a.begin;
    r1 = a.end;
    return !a.empty;
}

void sectorAdjacentUnion( const Angle& a0, const Angle& a1,
                          const Angle& b0, const Angle& b1,
                          Angle& r0, Angle& r1) {
    using namespace std;
    Sector a (a0, a1), b (b0, b1);
    a.adjacentUnion( b );
    r0 = a.begin;
    r1 = a.end;
}

Sector::Sector(const Angle& begin, const Angle& end) :
    begin ( begin ),
    end ( end ),
    empty ( false )
{
    recheckBranchCut();
}

void Sector::recheckBranchCut(void) {
    containsBranchCut = ( !empty && end < begin );
}

void HexFovNorthBeam::setLighted(int x, int y) {
    receiver.setLighted( x, y );
}

HexFovNorthBeam::HexFovNorthBeam( HexOpacityMap& map, HexLightReceiver& receiver ) :
    map ( map ),
    cx ( 0 ), // todo
    cy ( 0 ),
    receiver ( receiver ),
    current ( new LightedTileQueue() ),
    primary ( new LightedTileQueue() ),
    secondary ( new LightedTileQueue() )
{
    current->add(0,2,Angle(1,1),Angle(-1,1));
}

HexFovNorthBeam::~HexFovNorthBeam(void) {
    delete current;
    delete primary;
    delete secondary;
}

}
