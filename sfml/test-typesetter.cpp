#include "typesetter.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>

#include <SFML/Graphics.hpp>

class RedFilter : public PixelFilter {
    ColRGBA operator()(ColRGBA col) const {
        return MAKE_COL( COL_RED(col), 0, 0, COL_ALPHA(col) );
    }
};

class DebugLineRenderer : public LineRenderer {
    public:
        void render(const FormattedLine& line) {
            using namespace std;
            cout << line.getRawText() << endl;
        }
};

int main(int argc, char *argv[]) {
    RedFilter filter;
    const ColRGBA vals[] = { MAKE_COL(255,255,255,255),
                             MAKE_COL(128,128,128,255),
                             MAKE_COL(0,0,0,0) };
    ImageBuffer buffer (640, 480);
    buffer.addFilter( &filter ); 
    for(int i=0;i<640;i++) {
        for(int j=0;j<480;j++) {
            int v = 0;
            if( i < j ) ++v;
            if( ((i-320)*(i-320) + (j-240)*(j-240)) < 10000 ) ++v; 
            buffer.setPixel( i, j, vals[v] );
        }
    }

    FreetypeLibrary lib;
    FreetypeFace myFont ("./data/Vera.ttf", 60);
    DebugLineRenderer dlr;
    WordWrapper wrapper ( dlr, 400, myFont.getWidthOf(' ') );
    sf::Color white (255,255,255);
    std::string text ("One does not simply ROCK into Mordor" );

    for(int i=0;i<text.size();i++) {
        wrapper.feed( FormattedCharacter( myFont, white, (uint32_t)text[i] ) );
    }
    wrapper.end();

    return 0;
}
