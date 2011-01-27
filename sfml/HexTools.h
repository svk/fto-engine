#ifndef H_HEXTOOLS
#define H_HEXTOOLS

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

};

#endif
