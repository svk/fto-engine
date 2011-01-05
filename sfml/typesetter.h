#ifndef H_TYPESETTER
#define H_TYPESETTER

#include <ft2build.h>
#include FT_FREETYPE_H


#include <vector>
#include <stdint.h>

typedef uint32_t ColRGBA;
#define COL_RED(col)   ((col&0xff000000)>>24)
#define COL_GREEN(col) ((col&0x00ff0000)>>16)
#define COL_BLUE(col)  ((col&0x0000ff00)>>8)
#define COL_ALPHA(col) ((col&0x000000ff)>>0)
#define MAKE_COL(r,g,b,a) (((r)<<24)|((g)<<16)|((b)<<8)|(a))


class PixelFilter {
    public:
        virtual ColRGBA operator()(ColRGBA) const = 0;
};

class ImageBuffer {
    // RGBA, the format loadable by 
    private:
        int width, height;
        ColRGBA *data;

        typedef std::vector<PixelFilter*> FilterList;
        FilterList filters;

        ColRGBA applyFilters( ColRGBA ) const;

    public:
        ImageBuffer(int,int);
        ~ImageBuffer(void);

        void addFilter( PixelFilter* );

        void setPixel(int,int, ColRGBA);
        void putFTGraymap(int, int, FT_Bitmap*);

        void writeP6(FILE *);
};

#endif
