#include "typesetter.h"

#include <ft2build.h>
#include FT_FREETYPE_H

class RedFilter : public PixelFilter {
    ColRGBA operator()(ColRGBA col) const {
        return MAKE_COL( COL_RED(col), 0, 0, COL_ALPHA(col) );
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

    FT_Library library;
    FT_Init_FreeType( &library );

    FT_Face face;
    FT_New_Face( library,
                 "./data/Vera.ttf",
                 0,
                 &face );
    FT_Set_Pixel_Sizes( face,
                        0,
                        300 );
    int x = 0, y = 300;
    FT_Load_Char( face, 'K', FT_LOAD_RENDER );
    buffer.putFTGraymap( x + face->glyph->bitmap_left,
                         y - face->glyph->bitmap_top,
                         &face->glyph->bitmap );
    x += face->glyph->advance.x >> 6;
    fprintf( stderr, "%d", x );
    FT_Load_Char( face, 'a', FT_LOAD_RENDER );
    buffer.putFTGraymap( x + face->glyph->bitmap_left,
                         y - face->glyph->bitmap_top,
                         &face->glyph->bitmap );


    buffer.writeP6( stdout );
}
