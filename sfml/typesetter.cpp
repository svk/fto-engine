#include "typesetter.h"

#include <stdexcept>

ImageBuffer::ImageBuffer(int width, int height) :
    width ( width ),
    height ( height ),
    data ( new ColRGBA [width*height] ),
    filters ()
{
}

void ImageBuffer::addFilter(PixelFilter* filter) {
    filters.push_back( filter );
}

ColRGBA ImageBuffer::applyFilters(ColRGBA col) const {
    FilterList::const_iterator i = filters.begin();
    while( i != filters.end() ) {
        col = (**i)(col);
        i++;
    }
    return col;
}

void ImageBuffer::setPixel(int x, int y, ColRGBA col) {
    if( x < 0 || y < 0 || x >= width || y >= height ) {
        return;
    }
    data[x + y * width] = applyFilters( col );
}

void ImageBuffer::putFTGraymap(int x0, int y0, FT_Bitmap *bitmap) {
    unsigned char *buffer = (bitmap->pitch < 0) ? &bitmap->buffer[(-bitmap->pitch)*(bitmap->rows-1)] : bitmap->buffer;
    for(int row=0;row<bitmap->rows;row++) {
        for(int col=0;col<bitmap->width;col++) {
            uint8_t gray = static_cast<uint8_t>( buffer[col] );
            uint32_t richGray = MAKE_COL( gray, gray, gray, 255 );
            setPixel( x0 + col, y0 + row, richGray );
        }
        buffer += bitmap->pitch;
    }
}

void fwrite_all(char *buffer, FILE *f, int len) {
    while( len > 0 ) {
        int written = fwrite( buffer, 1, len, f );
        if( !written ) return;
        if( written < 0 ) {
            throw std::runtime_error( "unexpected write error" );
        }
        len -= written;
        buffer += written;
    }
}

void ImageBuffer::writeP6(FILE *f) {
    char buffer[1024];
    sprintf( buffer, "P6\n%d %d\n255\n", width, height);
    fwrite_all( buffer, f, strlen(buffer) );
    for(int y=0;y<height;y++) {
        for(int x=0;x<width;x++) {
            ColRGBA col = data[x + y * width];
            buffer[0] = COL_RED( col );
            buffer[1] = COL_GREEN( col );
            buffer[2] = COL_BLUE( col );
            fwrite_all( buffer, f, 3 );
        }
    }
}

ImageBuffer::~ImageBuffer(void) {
    delete [] data;
}
