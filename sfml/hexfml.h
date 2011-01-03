#ifndef H_HEXFML
#define H_HEXFML

#include <SFML/Graphics/Image.hpp>

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

        void screenToHex( int&, int&, int, int );
        void hexToScreen( int&, int& );

};

#endif
