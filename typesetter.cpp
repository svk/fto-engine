#include "typesetter.h"

#include <stdexcept>

#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cstring>

#include <cassert>

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

#define TO_PIXELS(i) (((i)+63)/64)

ImageBuffer::ImageBuffer(int width, int height) :
    width ( width ),
    height ( height ),
    data ( new ColRGBA [width*height] )
{
    memset( data, 0, width * height * sizeof *data );
}

void ImageBuffer::setPixel(int x, int y, ColRGBA col) {
    if( x < 0 || y < 0 || x >= width || y >= height ) {
        return;
    }
    data[x + y * width] = col;
}

void ImageBuffer::putFTGraymap(int x0, int y0, FT_Bitmap *bitmap, const sf::Color& colour ) {
    unsigned char *buffer = (bitmap->pitch < 0) ? &bitmap->buffer[(-bitmap->pitch)*(bitmap->rows-1)] : bitmap->buffer;
    for(int row=0;row<bitmap->rows;row++) {
        for(int col=0;col<bitmap->width;col++) {
            uint8_t gray = static_cast<uint8_t>( buffer[col] );
            uint32_t notGray = MAKE_COL( colour.r, colour.g, colour.b, gray );
            if( gray == 0 ) continue; // hax! not true blending
            setPixel( x0 + col, y0 + row, notGray );
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
            buffer[0] = (COL_RED( col ) * COL_ALPHA(col)) / 255;
            buffer[1] = (COL_GREEN( col ) * COL_ALPHA(col)) / 255;
            buffer[2] = (COL_BLUE( col ) * COL_ALPHA(col)) / 255;
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

FormattedCharacter::FormattedCharacter(FreetypeFace& face, const sf::Color& colour, uint32_t character) :
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
    maxAscender ( 0 ),
    components ()
{
}

FormattedWord::FormattedWord(void) :
    width ( 0 ),
    height ( 0 ),
    maxAscender ( 0 ),
    components ()
{
}

FormattedWord::FormattedWord(const FormattedWord& that) :
    width ( that.width ),
    height ( that.height ),
    maxAscender ( that.maxAscender ),
    components ( that.components )
{
}

const FormattedWord& FormattedWord::operator=(const FormattedWord& that) {
    if( this != &that ) {
        width = that.width;
        height = that.height;
        components = that.components;
        maxAscender = that.maxAscender;
    }
    return *this;
}

void FormattedWord::addCharacter(const FormattedCharacter& ch) {
    components.push_back( ch );
    width += components.back().getWidth();
    height = MAX( height, components.back().getHeight() );
    maxAscender = MAX( maxAscender, components.back().getAscender() );
}

void FormattedLine::addWord(const FormattedWord& wo) {
    components.push_back( wo );
    wordWidth += wo.getWidth();
    height = MAX( height, wo.getHeight() );
    maxAscender = MAX( maxAscender, wo.getAscender() );
}

void FormattedLine::clear(void) {
    wordWidth = height = maxAscender = 0;
    components.clear();
}

void FormattedWord::clear(void) {
    width = height = maxAscender = 0;
    components.clear();
}

int FormattedLine::getBreaks(void) const {
    if( components.size() < 1 ) return 0;
    return components.size() - 1;
}

int FormattedCharacter::getWidth(void) {
    return face->getWidthOf( character );
}

int FormattedCharacter::getAscender(void) {
    return TO_PIXELS(face->getFace()->size->metrics.ascender);
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
            endCurrentLine(false);
            break;
        case TSCT_BREAK:
            handleNewWord();
            break;
    }
}

void WordWrapper::end(void) {
    handleNewWord();
    endCurrentLine(false);
}

void WordWrapper::endCurrentLine(bool broken) {
    renderer.render( currentLine, !broken );
    currentLine.clear();
}

int FormattedLine::getWidthWithSpacing(int spacing) const {
    return getWordWidth() +
           spacing * getBreaks();
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
    endCurrentLine(true);
    currentLine.addWord( currentWord );
    currentWord.clear();
}

// not optimized at all -- all this just for width/height, independently!?

int FreetypeFace::getWidthOf(uint32_t ch) {
    int error = FT_Load_Char( face, ch, FT_LOAD_RENDER );
    if( error ) return 0;
    return TO_PIXELS(face->glyph->advance.x);
}

int FreetypeFace::getHeight(void) {
    return TO_PIXELS(face->size->metrics.height);
}

int FreetypeFace::getHeightOf(uint32_t ch) {
    using namespace std;
    return TO_PIXELS(face->size->metrics.height);
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

int FormattedCharacter::render(int x, int y, ImageBuffer& buffer) const {
    using namespace std;
    int error = FT_Load_Char( face->getFace(), character, FT_LOAD_RENDER );
    if( error ) return 0;
    buffer.putFTGraymap(x + face->getFace()->glyph->bitmap_left,
                        y - face->getFace()->glyph->bitmap_top,
                        &face->getFace()->glyph->bitmap,
                        colour );
    return TO_PIXELS(face->getFace()->glyph->advance.x);
}

int FormattedWord::render(int x, int y, ImageBuffer& buffer) const {
    int dx = 0;
    using namespace std;
    for(FCList::const_iterator i = components.begin(); i != components.end(); i++) {
        dx += i->render( x + dx, y, buffer );
    }
    return dx;
}

void FormattedLine::renderLeftJustified(int x, int y, int spacing,ImageBuffer& buffer) const {
    int dx = 0;
    using namespace std;
    for(FWList::const_iterator i = components.begin(); i != components.end(); i++) {
        dx += i->render( x + dx, y + maxAscender, buffer );
        dx += spacing;
    }
}

void FormattedLine::renderCentered(int x, int y, int spacing, int width, ImageBuffer& buffer) const {
    int surplus = MAX(0, width - getWidthWithSpacing(spacing) );
    renderLeftJustified(x + surplus/2, y, spacing, buffer );
}

void FormattedLine::renderPadded(int x, int y, int spacing, int width, ImageBuffer& buffer) const {
    int surplus = MAX(0, width - getWidthWithSpacing(spacing) );
    int low = surplus / getBreaks(), high = (surplus + getBreaks() - 1) / getBreaks();
    int dx = 0;
    int index = 0;
    using namespace std;
    for(FWList::const_iterator i = components.begin(); i != components.end(); i++) {
        dx += i->render( x + dx, y + maxAscender, buffer );
        dx += spacing;
        if( (index%2) == 0 ) {
            dx += MIN( surplus, low );
            surplus -= low;
        } else {
            dx += MIN( surplus, high );
            surplus -= high;
        }
        index++;
    }
}

sf::Color ImageBuffer::getColourAt(int x, int y) {
    if( x < 0 || y < 0 || x >= width || y >= height ) {
        throw std::out_of_range( "pixel position out of image buffer range" );
    }
    uint32_t rgba = data[ x + y * width ];
    return sf::Color( COL_RED(rgba), COL_GREEN(rgba), COL_BLUE(rgba), COL_ALPHA(rgba) );
}

SfmlRectangularRenderer::SfmlRectangularRenderer(int width, int spacing, TextJustifyMode mode) :
    x ( 0 ),
    totalHeight ( 0 ),
    width ( width ),
    spacing ( spacing ),
    mode ( mode )
{
}

SfmlRectangularRenderer::~SfmlRectangularRenderer(void) {
    for(LineList::iterator i = lines.begin(); i != lines.end(); i++) {
        delete *i;
    }
}

sf::Image* SfmlRectangularRenderer::createImage(void) {
    int y = 0;
    sf::Image *rv = new sf::Image( width, totalHeight );
    for(LineList::const_iterator i = lines.begin(); i != lines.end(); i++) {
        int h = (*i)->getHeight();
        for(int x=0;x<width;x++) for(int dy=0;dy<h;dy++) {
            rv->SetPixel(x, y+dy, (*i)->getColourAt(x,dy) );
        }
        y += h;
    }
    return rv;
}

void SfmlRectangularRenderer::render(const FormattedLine& line, bool brokenAbnormally) {
    ImageBuffer *rv = new ImageBuffer(width, line.getHeight());
    switch( mode ) {
        case TJM_PAD:
            if( !brokenAbnormally ) {
                line.renderPadded( x, 0, spacing, width, *rv );
                break;
            }
            // fallthrough to left-justified for non-wrapped lines
        case TJM_LEFT:
            line.renderLeftJustified( x, 0, spacing, *rv );
            break;
        case TJM_CENTER:
            line.renderCentered( x, 0, spacing, width, *rv );
            break;
    }
    totalHeight += line.getHeight();
    lines.push_back( rv );
}

void SfmlRectangularRenderer::setWidth(int width_) {
    width = width_;
}

LabelSprite::LabelSprite(const std::string& text, sf::Color colour, FreetypeFace& face) {
    const int spacing = face.getWidthOf( ' ' );
    FormattedWord word;
    FormattedLine line;
    for(int i=0;i<(int)text.length();i++) {
        if( isspace( text[i] ) ) {
            if( word.getLength() > 0 ) {
                line.addWord( word );
                word.clear();
            }
        } else {
            FormattedCharacter ch ( face, colour, text[i] );
            word.addCharacter( ch );
        }
    }
    if( word.getLength() > 0 ) {
        line.addWord( word );
        word.clear();
    }
    construct( spacing, line );
}

LabelSprite::LabelSprite(FreetypeFace& face, const FormattedLine& line) {
    construct( face.getWidthOf( ' ' ), line );
}

void LabelSprite::construct(int spacing, const FormattedLine& line) {
    SfmlRectangularRenderer renderer ( -1, spacing, TJM_LEFT );
    renderer.setWidth( line.getWidthWithSpacing( spacing ) );
    renderer.render( line, false );
    image = renderer.createImage();
    image->SetSmooth( false );
    sprite = new sf::Sprite();
    sprite->SetImage( *image );
    setPosition(0,0);
}

LabelSprite::~LabelSprite(void) {
    delete sprite;
    delete image;
}

void LabelSprite::setPosition(int x, int y) {
    sprite->SetPosition( 0.5 + (double) x, 0.5 + (double) y );
}

void LabelSprite::draw(sf::RenderWindow& win) const {
    win.Draw( *sprite );
}

int LabelSprite::getHeight(void) const {
    return (int)(0.999 + sprite->GetSize().y);
}

int LabelSprite::getWidth(void) const {
    return (int)(0.999 + sprite->GetSize().x);
}

ChatLineSprite::ChatLineSprite(int width, const ChatLine& chatline, FreetypeFace& face) :
    nameSprite ( 0 )
{
    construct( width, chatline.username, chatline.usernameColour, chatline.text, chatline.textColour, face );
}

ChatLineSprite::ChatLineSprite(int width, const std::string& name, sf::Color usernameColour, const std::string& text, sf::Color textColour, FreetypeFace& face) :
    nameSprite ( 0 )
{
    construct( width, name, usernameColour, text, textColour, face );
}

int ChatLineSprite::getOffset(void) const {
    using namespace std;
    if( nameSprite ) {
        return nameSprite->getWidth() + padding;
    }
    return 0;
}

void ChatLineSprite::construct(int width, const std::string& name, sf::Color usernameColour, const std::string& text, sf::Color textColour, FreetypeFace& face) {
    padding = 10;
    if( name.length() > 0 ) {
        nameSprite = new LabelSprite( name, usernameColour, face );
    }
    int textWidth = width - getOffset();
    int spacing = face.getWidthOf(' ');
    SfmlRectangularRenderer rr ( textWidth, spacing, TJM_LEFT );
    WordWrapper wrap ( rr, textWidth, spacing );
    for(int i=0;i<(int)text.size();i++) {
        wrap.feed( FormattedCharacter( face, textColour, (uint32_t) text[i] ) );
    }
    wrap.end();
    image = rr.createImage();
    textSprite = new sf::Sprite();
    textSprite->SetImage( *image );
    setPosition(0,0);
    if( nameSprite ) {
        height = MAX( nameSprite->getHeight(), (int) image->GetHeight() );
    } else {
        height = image->GetHeight();
    }
}

ChatLineSprite::~ChatLineSprite(void) {
    if( nameSprite ) {
        delete nameSprite;
    }
    delete textSprite;
    delete image;
}

int ChatLineSprite::getHeight(void) const {
    return height;
}

void ChatLineSprite::setPosition(int x, int y) {
    using namespace std;
    if( nameSprite ) {
        nameSprite->setPosition( x, y );
    }
    textSprite->SetPosition( 0.5 + (double) (x + getOffset()),
                             0.5 + (double) y );
}

void ChatLineSprite::draw(sf::RenderWindow& win) const {
    if( nameSprite ) {
        nameSprite->draw( win );
    }
    win.Draw( *textSprite );
}

ChatBox::ChatBox(int x, int y, int width, int height, FreetypeFace& face, sf::Color colour ) :
    face ( face ),
    bgColour ( colour ),
    x ( x ),
    y ( y ),
    width ( width ),
    height ( height )
{
}

ChatLine::ChatLine(const std::string& username, sf::Color usernameColour,
                   const std::string& text, sf::Color textColour) :
    username ( username ),
    usernameColour ( usernameColour ),
    text ( text ),
    textColour ( textColour )
{
}

void ChatBox::resize(int width_, int height_) {
    if( width != width_ ) {
        clearCache();
    }
    width = width_;
    height = height_;
    rebuildCache();
}

ChatLineSprite* ChatBox::render(const ChatLine& chatline) {
    return new ChatLineSprite( width, chatline, face );
}

void ChatBox::clearCache(void) {
    for(SpriteList::iterator i = renderedLinesCache.begin(); i != renderedLinesCache.end(); i++) {
        delete *i;
    }
    renderedLinesCache.clear();
}

ChatBox::~ChatBox(void) {
    clearCache();
}

void ChatBox::add(const ChatLine& line ) {
    ChatLineSprite *sprite = render( line );
    chatlines.push_back( line );
    renderedLinesCache.push_front( sprite );
}

void ChatBox::rebuildCache(void) {
    int remainingHeight = height;
    ChatLineList::reverse_iterator j = chatlines.rbegin();
    for(SpriteList::iterator i = renderedLinesCache.begin(); i != renderedLinesCache.end(); i++) {
        remainingHeight -= (*i)->getHeight();
        assert( j != chatlines.rend() );
        j++;
    }
    while( remainingHeight > 0 && j != chatlines.rend() ) {
        ChatLineSprite *sprite = render( *j );
        renderedLinesCache.push_back( sprite );
        remainingHeight -= sprite->getHeight();
        j++;
    }
}

void ChatBox::setPosition(int x_, int y_) {
    x = x_;
    y = y_;
}

void ChatBox::draw(sf::RenderWindow& win) {
    int remainingHeight = height;
    int currentY = y + height;
    SpriteList::iterator i = renderedLinesCache.begin();

    glScissor( x, win.GetHeight() - (y + height), width, height );
    glEnable( GL_SCISSOR_TEST );
    win.Clear( bgColour );

    while( i != renderedLinesCache.end() && remainingHeight >= 0 ) {
        ChatLineSprite *sprite = *i;
        currentY -= sprite->getHeight();
        remainingHeight -= sprite->getHeight();
        sprite->setPosition( x, currentY );
        sprite->draw( win );
        i++;
    }
    
    glDisable( GL_SCISSOR_TEST );
}

void LineBuilder::setCaret(const FormattedCharacter& caret_) {
    useCaret = true;
    if( caret ) {
        delete caret;
    }
    caret = new FormattedCharacter( caret_ );
}

void LineBuilder::addCharacter(const FormattedCharacter& ch) {
    characters.push_back( ch );
}

void LineBuilder::backspace(void) {
    if( characters.size() > 0 ) {
        characters.pop_back();
    }
}

FormattedLine LineBuilder::createLine(void) {
    FormattedWord word;
    FormattedLine line;
    for(std::vector<FormattedCharacter>::iterator i = characters.begin(); i != characters.end(); i++) {
        if( isspace( i->character ) ) {
            if( word.getLength() > 0 ) {
                line.addWord( word );
                word.clear();
            }
        } else {
            word.addCharacter( *i );
        }
    }
    if( useCaret ) {
        word.addCharacter( *caret );
    }
    if( word.getLength() > 0 ) {
        line.addWord( word );
        word.clear();
    }
    return line;
}

void LabelSprite::noRestrictToWidth(void) {
    sprite->SetSubRect( sf::IntRect(0,0,image->GetWidth(), image->GetHeight() ) );
}

void LabelSprite::restrictToWidth(int widthRestriction) {
    int imw = image->GetWidth();
    if( imw > widthRestriction ) {
        int offset = imw - widthRestriction;
        sprite->SetSubRect( sf::IntRect(offset,0,offset + widthRestriction,image->GetHeight() ) );
    } else {
        noRestrictToWidth();
        sprite->SetSubRect( sf::IntRect(0,0,image->GetWidth(), image->GetHeight() ) );
    }
}

void ChatInputLine::update(void) {
    if( sprite ) {
        delete sprite;
    }
    sprite = new LabelSprite( face, builder.createLine() );
    sprite->restrictToWidth( width );
    sprite->setPosition( x, y );
}

ChatInputLine::ChatInputLine(int width, FreetypeFace& face, sf::Color colour, FormattedCharacter caret) :
    width ( width ),
    colour ( colour ),
    face ( face ),
    sprite ( 0 ),
    ready ( false ),
    x(0),
    y(0)
{
    builder.setCaret( caret );
    update();
}

void ChatInputLine::add(uint32_t ch) {
    builder.addCharacter( FormattedCharacter( face, colour, ch ) );
    update();
}

void ChatInputLine::backspace(void) {
    builder.backspace();
    update();
}

void ChatInputLine::textEntered( uint32_t ch ) {
    const uint32_t codeBackspace = 8,
                   codeNewline = 13;
    if( ready ) return;
    if( ch == codeBackspace ) {
        backspace();
    } else if ( ch == codeNewline ) {
        if( getString().length() > 0 ) {
            ready = true;
        }
    } else if( isprint( ch ) ){
        add( ch );
    }
}

std::string ChatInputLine::getString(void) {
    std::string rv = builder.createLine().getRawText();
    return rv.substr( 0, rv.length() - 1 );
}

void ChatInputLine::setPosition(int x_,int y_) {
    x = x_;
    y = y_;
}

void ChatInputLine::draw(sf::RenderWindow& win) {
    sprite->setPosition( x, y );
    sprite->draw( win );
}

ChatInputLine::~ChatInputLine(void) {
    if( sprite ) {
        delete sprite;
    }
}

LineBuilder::LineBuilder(void) :
    characters (),
    useCaret ( false ),
    caret (0)
{
}

LineBuilder::~LineBuilder(void) {
    if( caret ) {
        delete caret;
    }
}

int ChatInputLine::getHeight(void) const {
    return sprite->getHeight();
}

void ChatInputLine::setWidth(int w) {
    width = w;
    update();
}

void LineBuilder::clear(void) {
    characters.clear();
}

void ChatInputLine::clear(void) {
    builder.clear();
    ready = false;
}
