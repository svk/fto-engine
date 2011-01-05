#include "typesetter.h"

#include <stdexcept>

#include <sstream>
#include <iostream>

#define MAX(a,b) (((a)>(b))?(a):(b))

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

FreetypeLibrary::FreetypeLibrary(void) {
    if( singleton ) {
        throw std::runtime_error( "freetype initialized twice" );
    }
    singleton = this;

    int error = FT_Init_FreeType( &library );
    if( error ) {
        throw std::runtime_error( "unable to initialize freetype" );
    }
}

FreetypeLibrary::~FreetypeLibrary(void) {
    singleton = 0;
    FT_Done_FreeType( library );
}

FreetypeLibrary* FreetypeLibrary::singleton = 0;

FT_Library FreetypeLibrary::getSingleton(void) {
    if( !singleton ) {
        throw std::runtime_error( "freetype not initialized" );
    }
    return singleton->library;
}

FreetypeFace::FreetypeFace(const std::string& fontName,
                           int pixelSize) :
    pixelSize(pixelSize)
{
    int error = FT_New_Face( FreetypeLibrary::getSingleton(),
                             fontName.c_str(),
                             0,
                             &face );
    if( error ) {
        throw std::runtime_error( "error loading font: " + fontName );
    }
    error = FT_Set_Pixel_Sizes( face,
                                0,
                                pixelSize );
    if( error ) {
        FT_Done_Face( face );
        throw std::runtime_error( "error setting pixel size for font: " + fontName );
    }
}

FreetypeFace::~FreetypeFace(void) {
    FT_Done_Face( face );
}

FormattedCharacter::FormattedCharacter(FreetypeFace& face, sf::Color& colour, uint32_t character) :
    face (&face),
    colour (colour),
    character (character)
{
}

FormattedCharacter::FormattedCharacter(const FormattedCharacter& that) :
    face (that.face),
    colour (that.colour),
    character (that.character)
{

}

const FormattedCharacter& FormattedCharacter::operator=(const FormattedCharacter& that) {
    if( this != &that ) {
        face = that.face;
        colour = that.colour;
        character = that.character;
    }
    return *this;
}

TsChartype classifyCharacter( uint32_t ch ) {
    switch( ch ) {
        case ' ':
            return TSCT_BREAK;
        case '\n':
            return TSCT_NEWLINE;
        case '\t':
        case '\r':
            return TSCT_IGNORE;
        default:
            return TSCT_NORMAL;
    }
}

FormattedLine::FormattedLine(void) :
    wordWidth ( 0 ),
    height ( 0 ),
    components ()
{
}

FormattedWord::FormattedWord(void) :
    width ( 0 ),
    height ( 0 ),
    components ()
{
}

FormattedWord::FormattedWord(const FormattedWord& that) :
    width ( that.width ),
    height ( that.height ),
    components ( that.components )
{
}

const FormattedWord& FormattedWord::operator=(const FormattedWord& that) {
    if( this != &that ) {
        width = that.width;
        height = that.height;
        components = that.components;
    }
    return *this;
}

void FormattedWord::addCharacter(const FormattedCharacter& ch) {
    components.push_back( ch );
    width += components.back().getWidth();
    height = MAX( height, components.back().getHeight() );
}

void FormattedLine::addWord(const FormattedWord& wo) {
    components.push_back( wo );
    wordWidth += wo.getWidth();
    height = MAX( height, wo.getHeight() );
}

void FormattedLine::clear(void) {
    wordWidth = height = 0;
    components.clear();
}

void FormattedWord::clear(void) {
    width = height = 0;
    components.clear();
}

int FormattedLine::getBreaks(void) const {
    if( components.size() < 1 ) return 0;
    return components.size() - 1;
}

int FormattedCharacter::getWidth(void) {
    return face->getWidthOf( character );
}

int FormattedCharacter::getHeight(void) {
    // is this right?
    return face->getHeightOf( character );
}

WordWrapper::WordWrapper(LineRenderer& renderer, int widthBound, int minimumWordSpacing) :
    renderer ( renderer ),
    widthBound ( widthBound ),
    minimumWordSpacing( minimumWordSpacing )
{
}

void WordWrapper::feed(const FormattedCharacter& ch) {
    switch( classifyCharacter( ch.character ) ) {
        case TSCT_NORMAL:
            currentWord.addCharacter( ch );
            break;
        case TSCT_IGNORE:
            break;
        case TSCT_NEWLINE:
            endCurrentLine();
            break;
        case TSCT_BREAK:
            handleNewWord();
            break;
    }
}

void WordWrapper::end(void) {
    currentLine.addWord( currentWord );
    endCurrentLine();
}

void WordWrapper::endCurrentLine(void) {
    renderer.render( currentLine );
    currentLine.clear();
}

void WordWrapper::handleNewWord(void) {
    int proposedWidth = currentLine.getWordWidth() +
                        minimumWordSpacing * (1 + currentLine.getBreaks()) +
                        currentWord.getWidth();
    using namespace std;
    if( proposedWidth < widthBound ) {
        currentLine.addWord( currentWord );
        currentWord.clear();
        return;
    }
    endCurrentLine();
    currentLine.addWord( currentWord );
    currentWord.clear();
}

// not optimized at all -- all this just for width/height, independently!?

int FreetypeFace::getWidthOf(uint32_t ch) {
    int error = FT_Load_Char( face, ch, FT_LOAD_RENDER );
    if( error ) return 0;
    return face->glyph->advance.x / 64;
}

int FreetypeFace::getHeightOf(uint32_t ch) {
    int error = FT_Load_Char( face, ch, FT_LOAD_RENDER );
    if( error ) return 0;
    return face->glyph->bitmap.rows;
}

std::string FormattedLine::getRawText(void) const {
    std::ostringstream oss;
    for(FWList::const_iterator i = components.begin(); i != components.end();) {
        oss << i->getRawText();
        i++;
        if( i != components.end() ) {
            oss << " ";
        }
    }
    return oss.str();
}

std::string FormattedWord::getRawText(void) const {
    std::ostringstream oss;
    for(FCList::const_iterator i = components.begin(); i != components.end();i++) {
        oss << (char) i->character;
    }
    return oss.str();
}
