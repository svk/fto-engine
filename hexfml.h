#ifndef H_HEXFML
#define H_HEXFML

#include <SFML/Graphics.hpp>

#include <iostream>

#include <stdint.h>

#include "HexTools.h"
using namespace HexTools; // temporary, to smoothly split hexfml.c

class HexSprite;

sf::FloatRect fitRectangleAt(double, double, const sf::FloatRect&, double, double);

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

        sf::Image* createSingleColouredImage( const sf::Color& ) const;
};


// the viewport stuff could probably easily be generalized to use other
// sorts of grids

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
        void drawGL(HexBlitter&, sf::RenderWindow&, double, double) const;

        bool translateCoordinates(int&, int&) const;
        
        void setBackgroundColour( const sf::Color& );
        void setNoBackgroundColour(void);

        void translateToHex(int,int,int,int,sf::View&) const;
        void beginClip(int,int);
        void endClip(void);
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
        HexSprite(const sf::Image&, const ScreenGrid&); // does not adopt!
        HexSprite(const sf::Sprite&, const ScreenGrid&);
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

#endif
