#include "typesetter.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>

#include <SFML/Graphics.hpp>

ImageBuffer *buffy;

class RedFilter : public PixelFilter {
    ColRGBA operator()(ColRGBA col) const {
        return MAKE_COL( COL_RED(col), 0, 0, COL_ALPHA(col) );
    }
};

class DebugLineRenderer : public LineRenderer {
    private:
        int x, y;
        int spacing;
    public:
        DebugLineRenderer(int x, int y, int spacing) :
            x(x),y(y),spacing(spacing)
        {
        }

        void render(const FormattedLine& line, bool brokenAbnormally) {
            using namespace std;
            cerr << "line " << line.getRawText() << " of length " << line.getWidthWithSpacing(spacing) << endl;
            if( brokenAbnormally ) {
                line.renderLeftJustified( x, y, spacing, *buffy );
            } else {
                line.renderPadded( x, y, spacing, 600, *buffy );
            }
            y += line.getHeight();
        }
};

int main(int argc, char *argv[]) {
    RedFilter filter;
    const ColRGBA vals[] = { MAKE_COL(255,255,255,255),
                             MAKE_COL(128,128,128,255),
                             MAKE_COL(0,0,0,0) };
    ImageBuffer buffer (640, 480);
    for(int i=0;i<640;i++) {
        for(int j=0;j<480;j++) {
            int v = 0;
            if( i < j ) ++v;
            if( ((i-320)*(i-320) + (j-240)*(j-240)) < 10000 ) ++v; 
            if( j > 300 ) v = 2;
            buffer.setPixel( i, j, vals[v] );
        }
    }

    FreetypeLibrary lib;
    FreetypeFace myFont ("./data/CrimsonText-Roman.otf", 20);
    FreetypeFace mySecondFont ("./data/CrimsonText-Bold.otf", 20);
    int spacing = myFont.getWidthOf(' ');
    DebugLineRenderer dlr (20, 300, spacing);
    WordWrapper wrapper ( dlr, 600, spacing );
    sf::Color white (255,255,255);
    sf::Color red (200,64,64);
    std::string text ("Each size object also contains a scaled versions of some of the global metrics described above. ");
    std::string moreText ("They can be accessed directly through the face->size->metrics structure. " );
    std::string evenMoreText ("Note that these values correspond to scaled versions of the design global metrics, with no rounding or grid-fitting performed. They are also completely independent of any hinting process. In other words, don't rely on them to get exact metrics at the pixel level. They are expressed in 26.6 pixel format." );

    buffy = &buffer;

    for(int i=0;i<text.size();i++) {
        wrapper.feed( FormattedCharacter( myFont, white, (uint32_t)text[i] ) );
    }
    for(int i=0;i<moreText.size();i++) {
        wrapper.feed( FormattedCharacter( mySecondFont, red, (uint32_t)moreText[i] ) );
    }
    for(int i=0;i<evenMoreText.size();i++) {
        wrapper.feed( FormattedCharacter( myFont, white, (uint32_t)evenMoreText[i] ) );
    }
    wrapper.end();

    buffer.writeP6( stdout );

    return 0;
}
