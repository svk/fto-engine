#ifndef H_HEXTOOLS
#define H_HEXTOOLS

#include <stdexcept>

#include <iostream>

#include <utility>

#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <stdexcept>
#include <cmath>

#include <iostream>

#include <set>

#include <map>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

namespace HexTools {

int hexCircleSize(int);
int flattenHexCoordinate(int,int);
void inflateHexCoordinate(int,int&,int&);
void polariseHexCoordinate(int,int,int&,int&,int&);
void cartesianiseHexCoordinate(int,int,int,int&,int&);

typedef std::pair<int,int> HexCoordinate;

class HexReceiver {
    public:
        virtual void add(int,int) = 0;
};

class HexRegion {
    public:
        typedef std::set< HexCoordinate > List;
        typedef List::const_iterator const_iterator;

    private:
        List coords;

    public:
        HexRegion(void) {}
        HexRegion(const HexRegion& that) :
            coords ( coords ) {}

        const HexRegion& operator=(const HexRegion& that) {
            if( this != &that ) {
                coords = that.coords;
            }
            return *this;
        }


        int size(void) const { return coords.size(); }
        void clear(void);
        void add(int,int);
        void remove(int,int);
        bool contains(int,int) const;


        const_iterator begin(void) const { return coords.begin(); }
        const_iterator end(void) const { return coords.end(); }
};

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
                size = that.size;
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

        int getSize(void) const { return size; }


        T& get(int k) {
            if( k < 0 || k >= size ) return getDefault();
            return tiles[k];
        }
        const T& get(int k) const {
            if( k < 0 || k >= size ) return getDefault();
            return tiles[k];
        }

        bool isDefault(int x, int y) const {
            return &get(x,y) == &defaultTile;
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

template<class T>
class SparseHexMap {
    private:
        T defaultValue;
        int maxRadius;
        std::map<HexCoordinate,T> data;
    
    public:
        explicit SparseHexMap(const T& defaultValue) :
            defaultValue( defaultValue ),
            maxRadius ( 0 ),
            data ()
        {
        }

        SparseHexMap(const SparseHexMap& h) :
            defaultValue ( h.defaultValue ),
            maxRadius ( h.maxRadius ),
            data ( h.data )
        {
        }

        int getMaxRadius(void) const {
            return maxRadius;
        }

        const SparseHexMap<T>& operator=(const SparseHexMap<T>& that) {
            using namespace std;
            if( this != &that ) {
                defaultValue = that.defaultValue;
                data = that.data;
                maxRadius = that.maxRadius;
            }
            return *this;
        }

        ~SparseHexMap(void) {
        }

        void set(int x, int y, const T& v) {
            int i, j, r;
            polariseHexCoordinate( x, y, i, j, r );
            maxRadius = MAX( maxRadius, r );
            data[HexCoordinate(x,y)] = v;
        }

        const T& get(int x, int y) const {
            typename std::map<HexCoordinate,T>::const_iterator i = data.find( HexCoordinate(x,y) );
            using namespace std;
            if( i == data.end() ) {
                return defaultValue;
            }
            return i->second;
        }
};

template<class T>
class DynamicHexMap {
    private:
        int size;
        T defaultTile;
        T *tiles;
    
    public:
        explicit DynamicHexMap(void) :
            size ( 1 ),
            tiles( new T [ size ] )
        {
            for(int i=0;i<size;i++) {
                using namespace std;
                tiles[i] = defaultTile;
            }
        }

        void extendTo(int i) {
            const int oldsize = size;
            T *rv = 0;
            int j = 0;
            using namespace std;
            if( i < size ) return;
            while( i >= size ) {
                size *= 2;
            }
            rv = new T [ size ];
            for(;j<oldsize;j++) {
                rv[j] = tiles[j];
            }
            for(;j<size;j++) {
                rv[j] = defaultTile;
            }
            delete [] tiles;
            tiles = rv;
        }

        DynamicHexMap(const DynamicHexMap& h) :
            size ( h.size ),
            defaultTile ( h.defaultTile ),
            tiles ( new T [ size ] )
        {
            for(int i=0;i<size;i++) {
                using namespace std;
                tiles[i] = h.tiles[i];
            }
        }

        const DynamicHexMap<T>& operator=(const DynamicHexMap<T>& that) {
            using namespace std;
            if( this != &that ) {
                size = that.size;
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

        ~DynamicHexMap(void) {
            delete [] tiles;
        }

        T& get(int k) {
            if( k < 0 || k >= size ) return getDefault();
            return tiles[k];
        }
        const T& get(int k) const {
            if( k < 0 || k >= size ) return getDefault();
            return tiles[k];
        }

        bool isDefault(int x, int y) const {
            return &get(x,y) == &defaultTile;
        }

        T& getDefault(void) { return defaultTile; }
        T& get(int x, int y) {
            using namespace std;
            int k = flattenHexCoordinate( x, y );
            if( k < 0 ) return getDefault();
            extendTo( k );
            return tiles[k];
        }

        const T& getDefault(void) const { return defaultTile; }
        const T& get(int x, int y) const {
            int k = flattenHexCoordinate( x, y );
            if( k < 0 || k >= size ) return getDefault();
            return tiles[k];
        }
};

bool isInvalidHexCoordinate(int,int);

};

#endif
