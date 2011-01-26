#ifndef H_HEXFML
#define H_HEXFML

#include <SFML/Graphics.hpp>

#include <iostream>

#include <stdint.h>

sf::FloatRect fitRectangleAt(double, double, const sf::FloatRect&, double, double);

int hexCircleSize(int);
int flattenHexCoordinate(int,int);
void inflateHexCoordinate(int,int&,int&);
void polariseHexCoordinate(int,int,int&,int&,int&);
void cartesianiseHexCoordinate(int,int,int,int&,int&);

class ScreenGrid {
    private:
        sf::Image hexPrototype;

        int width, height;

        // Exact pixel counts; not subject to rounding.
        // Counted from the top left.
        int hOffsetIncrement; // 2/3 hex width
        int vOffsetIncrement; // 1/2 hex width

        const uint32_t *pixels;
        uint32_t mainColour,
                 northwestColour,
                 northeastColour,
                 southwestColour,
                 southeastColour;

        
        void screenToGeometric( int&, int&, int, int ) const;
        void geometricToScreen( int&, int&, int, int ) const;

        int getColumn( int ) const;
        int getRow( int, int ) const;
        void cellClassify( int&, int&, int, int ) const;

        void analysePrototype(void);

    public:
        explicit ScreenGrid( const std::string& );
        ~ScreenGrid(void);

        void screenToHex( int&, int&, int, int ) const;
        void hexToScreen( int&, int& ) const;

        int getHexWidth(void) const { return width; }
        int getHexHeight(void) const { return height; }

        void centerRectangle(sf::FloatRect&) const;
};

class HexBlitter {
    public:
        virtual void drawHex(int, int, sf::RenderWindow&) = 0;
};

class HexViewport {
    // Note: this viewport is 1:1 and does not support
    // zoom -- this is intentional to maintain a
    // one to one pixel mapping.
    // This is also why integers are used for coordinates
    // throughout.

    private:
        const ScreenGrid& grid;

        int screenXOffset, screenYOffset;
        int screenWidth, screenHeight;

        int centerX, centerY;

        bool drawBackground;
        sf::Color bgColor;

    public:
        HexViewport(const ScreenGrid&,int,int,int,int);

        void setRectangle(int,int,int,int);
        void center(int,int);

        int getCenterX(void) { return centerX; }
        int getCenterY(void) { return centerY; }

        void draw(HexBlitter&, sf::RenderWindow&, sf::View&) const;

        bool translateCoordinates(int&, int&) const;
        
        void setBackgroundColour( const sf::Color& );
        void setNoBackgroundColour(void);
};

class HexSprite {
    private:
        const ScreenGrid& grid;

        sf::Image *image;
        sf::Sprite sprite;

        int offsetX, offsetY;

        int hx, hy;

        void loadSpriteFrom(const sf::Image&);

    public:
        HexSprite(const std::string&, const ScreenGrid&);
        HexSprite(const sf::Image&, const ScreenGrid&);
        ~HexSprite(void);

        void setPosition(int,int);
        void getPosition(int&,int&) const;

        void draw(sf::RenderWindow&) const;
};

class ViewportMouseScroller {
    private:
        HexViewport& vp;
        const sf::Input& input;

        int mouseX0, mouseY0;
        int vpX0, vpY0;

    public:
        ViewportMouseScroller( HexViewport&, const sf::Input& );

        void scroll(void);
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

#endif
