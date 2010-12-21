#include "Angelic.h"

namespace Angelic {

    Coordinate::Coordinate(int x, int y) :
        x ( x ),
        y ( y )
    {
    }

    int Coordinate::quadrant(void) const {
        if( x >= 0 ) {
            if( y >= 0 ) return 0;
            else return 3;
        } else {
            if ( y >= 0 ) return 1;
            else return 2;
        }
    }

    int Coordinate::compare(const Coordinate& that) const {
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

    bool Coordinate::operator>(const Coordinate& that) const {
        return compare( that ) > 0;
    }

    bool Coordinate::operator<(const Coordinate& that) const {
        return compare( that ) < 0;
    }

    bool Coordinate::operator>=(const Coordinate& that) const {
        return compare( that ) >= 0;
    }

    bool Coordinate::operator<=(const Coordinate& that) const {
        return compare( that ) <= 0;
    }

    bool Coordinate::operator==(const Coordinate& that) const {
        return compare( that ) <= 0;
    }

}
