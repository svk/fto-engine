#ifndef H_HEXTOOLS
#define H_HEXTOOLS

#include <stdexcept>

namespace HexTools {

int hexCircleSize(int);
int flattenHexCoordinate(int,int);
void inflateHexCoordinate(int,int&,int&);
void polariseHexCoordinate(int,int,int&,int&,int&);
void cartesianiseHexCoordinate(int,int,int,int&,int&);

template<class T>
class HexMap {
    private:
        int radius, size;
        T defaultTile;
        T *tiles;
    
    public:
        explicit HexMap(const int radius) :
            radius ( radius ),
            size ( hexCircleSize(radius) ),
            tiles( new T [ size ] )
        {
        }

        HexMap(const HexMap& h) :
            radius ( h.radius ),
            size ( h.size ),
            defaultTile ( h.defaultTile ),
            tiles ( new T [ size ] )
        {
            for(int i=0;i<size;i++) {
                using namespace std;
                tiles[i] = h.tiles[i];
            }
        }

        const HexMap<T>& operator=(const HexMap<T>& that) {
            using namespace std;
            if( this != &that ) {
                radius = that.radius;
                size = that.radius;
                defaultTile = that.defaultTile;
                if( tiles ) {
                    delete [] tiles;
                }
                tiles = new T [ size ];
                for(int i=0;i<size;i++) {
                    tiles[i] = that.tiles[i];
                }
            }
            return *this;
        }

        ~HexMap(void) {
            delete [] tiles;
        }

        T& getDefault(void) { return defaultTile; }
        T& get(int x, int y) {
            int k = flattenHexCoordinate( x, y );
            if( k < 0 || k >= size ) return getDefault();
            return tiles[k];
        }

        const T& getDefault(void) const { return defaultTile; }
        const T& get(int x, int y) const {
            int k = flattenHexCoordinate( x, y );
            if( k < 0 || k >= size ) return getDefault();
            return tiles[k];
        }
};

template<class T>
class HexTorusMap {
    private:
        int radius;
        HexMap<T> submap;

        void toCore(int& x, int& y) {
            int guard = 100;
            int i, j, r;
            polariseHexCoordinate( x, y, i, j, r );
            if( r < radius ||
                ( (r == radius) &&
                  ( (i == 0 && j != 0) ||
                    (i == 1) ||
                    (i == 2) ) ) ) {
                return;
            }
            if( r == radius ) switch( i ) {
                case 0:
                case 5:
                    x *= -1;
                    toCore( x, y );
                    return;
                case 3:
                    cartesianiseHexCoordinate( 0, radius - j, r, x, y );
                    toCore( x, y );
                    return;
                case 4:
                    cartesianiseHexCoordinate( 1, radius - j, r, x, y );
                    toCore( x, y );
                    return;
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
                    toCore(x,y);
                    return;
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

    public:
        explicit HexTorusMap(const int radius) :
            radius ( radius ),
            submap ( radius )
        {
        }

        T& get(int x, int y) {
            toCore(x,y);
            return submap.get(x,y);
        }

        const T& get(int x, int y) const {
            toCore(x,y);
            return submap.get(x,y);
        }
};

};

#endif
