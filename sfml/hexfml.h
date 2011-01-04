#ifndef H_HEXFML
#define H_HEXFML

#include <SFML/Graphics.hpp>

#include <stdint.h>

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
        ScreenGrid( const std::string& );
        ~ScreenGrid(void);

        void screenToHex( int&, int&, int, int ) const;
        void hexToScreen( int&, int& ) const;

        int getHexWidth(void) const { return width; }
        int getHexHeight(void) const { return height; }

        void centerRectangle(sf::FloatRect&);
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

    public:
        HexViewport(const ScreenGrid&,int,int,int,int);

        void setRectangle(int,int,int,int);
        void center(int,int);

        void draw(HexBlitter&, sf::RenderWindow&, sf::View&) const;
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

#endif
