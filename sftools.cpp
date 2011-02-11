#include "sftools.h"

#include <iostream>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

sf::Color ToTranslucent::transform(const sf::Color& x) {
    return sf::Color( x.r, x.g, x.b, alpha );
}

sf::Color ToGrayscale::transform(const sf::Color& x) {
    int rgb = (int)(0.5 + ((double)(x.r + x.g + x.b))/3.0);
    return sf::Color( rgb, rgb, rgb, x.a );
}

sf::Image* PixelTransform::apply(sf::Image *rv) {
    const int w = rv->GetWidth(), h = rv->GetHeight();
    for(int x=0;x<w;x++) for(int y=0;y<h;y++) {
        rv->SetPixel( x, y, transform( rv->GetPixel(x, y ) ) );
    }
    return rv;
}

SfmlEventHandler::SfmlEventHandler(void) :
    next ( 0 )
{
}

void SfmlEventHandler::setNextEventHandler(SfmlEventHandler *next_) {
    next = next_;
}

bool SfmlEventHandler::delegate(const sf::Event& event) {
    if( next ) return next->handleEvent( event );
    return false;
}

bool CompositeEventHandler::handleEvent(const sf::Event& ev) {
    for(HandlerList::iterator i = handlers.begin(); i != handlers.end(); i++) {
        if( (*i)->handleEvent( ev ) ) return true;
    }
    return delegate( ev );
}

CompositeEventHandler::CompositeEventHandler(void) {
}

CompositeEventHandler::~CompositeEventHandler(void) {
    for(HandlerList::iterator i = handlers.begin(); i != handlers.end(); i++) {
        delete *i;
    }
}

void SfmlApplication::processIteration(void) {
    sf::Event ev;
    double dt = clock.GetElapsedTime();
    clock.Reset();
    if( dt > 0 ) {
        tick( dt );
    }
    while( window.GetEvent( ev ) ) {
        handleEvent( ev );
    }
    if( currentScreen ) {
        window.SetView( sf::View( sf::FloatRect( 0, 0, width, height ) ) );
        currentScreen->draw( window );
    }
    window.Display();
}

void SfmlApplication::stop(void) {
    keepRunning = false;
}

void SfmlApplication::run(void) {
    while( keepRunning && window.IsOpened() ) {
        processIteration();
    }
}

void CompositeEventHandler::adoptHandler(SfmlEventHandler* handler) {
    handlers.push_back( handler );
}

void SfmlApplication::resize(int width_, int height_) {
    width = width_;
    height = height_;
    mainView = sf::View( sf::Vector2f(0,0),
                         sf::Vector2f( width/2.0, height / 2.0 ) );
    currentScreen->resize(width, height);
}

bool SfmlApplication::handleEvent(const sf::Event& ev) {
    switch( ev.Type ) {
        case sf::Event::Resized:
            resize( ev.Size.Width, ev.Size.Height );
            break;
        case sf::Event::Closed:
            window.Close();
            break;
        default:
            return delegate(ev);
    }
    return true;
}

void SfmlApplication::setScreen(SfmlScreen* screen) {
    currentScreen = screen;
    currentScreen->resize( width, height );
    setNextEventHandler( currentScreen );
}

SfmlApplication::SfmlApplication(std::string title, int width, int height) :
    width (width),
    height (height),
    clock (),
    currentScreen ( 0 ),
    keepRunning ( true ),
    window ( sf::VideoMode( width, height, 32 ), title )
{
    window.SetFramerateLimit( 60 );
    window.UseVerticalSync( true );
}

SfmlApplication::~SfmlApplication(void) {
}

Spritesheet::Spritesheet(int width, int height) :
    sheet ( width,height, sf::Color(254,254,0)),
    width ( width ),
    height ( height ),
    x ( 0 ),
    y ( 0 ),
    rowHeight ( 0 ),
    rects ()
{
    sheet.SetSmooth( false );
}

bool Spritesheet::hasSpaceFor(int sw, int sh) const {
    if( sw <= (width-x) && sh <= (height-y) ) {
        return true;
    }
    if( sw <= width && sh <= (height-y-rowHeight) ) {
        return true;
    }
    return false;
}

int Spritesheet::adopt(sf::Image* img) {
    const int pad = 1;
    const int sw = img->GetWidth(), sh = img->GetHeight();
    int rv;
    if( !hasSpaceFor( sw + 2*pad, sh + 2*pad ) ) {
        throw SpritesheetFull();
    }
    if( (sw+2*pad) > (width-x) ) {
        y += rowHeight;
        x = 0;
        rowHeight = 0;
    }
    rowHeight = MAX( sh + 2 * pad, rowHeight );
    rv = rects.size();
    sf::IntRect rect ( x + pad, y + pad, x + pad + sw, y + pad + sh );
    rects.push_back( rect );
    for(int sx=0;sx<(sw+2*pad);sx++) for(int sy=0;sy<(sh+2*pad);sy++) {
        sheet.SetPixel( x + sx, y + sy, sf::Color(255,0,255,0) );
    }
    for(int sx=0;sx<sw;sx++) for(int sy=0;sy<sh;sy++) {
        sheet.SetPixel( x + pad + sx, y + pad + sy, img->GetPixel( sx, sy ) );
    }
    x += width + 2 * pad;
    delete img;
    return rv;
}

sf::Sprite Spritesheet::makeSprite(int j) const {
    sf::Sprite rv;
    rv.SetImage( sheet );
    rv.SetSubRect( rects[j] );
    return rv;
}



sf::SoundBuffer* loadSoundBufferFromFile(const std::string& fn) {
    sf::SoundBuffer *rv = new sf::SoundBuffer();
    if( !rv->LoadFromFile( fn ) ) {
        delete rv;
        return 0;
    }
    return rv;
}


sf::Image* loadImageFromFile(const std::string& fn) {
    sf::Image *rv = new sf::Image();
    if( !rv->LoadFromFile( fn ) ) {
        delete rv;
        return 0;
    }
    rv->SetSmooth( false );
    return rv;
}

void Spritesheet::bindTexture(void) {
    sheet.Bind();
}

void drawSprite( const sf::Sprite& sprite ) {
    sprite.GetImage()->Bind();
    drawBoundSprite( sprite );
}

void drawSpriteCentered( const sf::Sprite& sprite, double spaceWidth, double spaceHeight ) {
    sprite.GetImage()->Bind();
    drawBoundSpriteCentered( sprite, spaceWidth, spaceHeight );
}

void drawBoundSprite( const sf::Sprite& sprite ) {
    const float width = sprite.GetSize().x, height = sprite.GetSize().y;
    const sf::FloatRect rect = sprite.GetImage()->GetTexCoords( sprite.GetSubRect() );
    using namespace std;
    glBegin( GL_QUADS );
    glTexCoord2f( rect.Left, rect.Top ); glVertex2f(0+0.5,0+0.5);
    glTexCoord2f( rect.Left, rect.Bottom ); glVertex2f(0+0.5,height+0.5);
    glTexCoord2f( rect.Right, rect.Bottom ); glVertex2f(width+0.5,height+0.5);
    glTexCoord2f( rect.Right, rect.Top ); glVertex2f(width+0.5,0+0.5);
    glEnd();
}

void drawBoundSpriteCentered( const sf::Sprite& sprite, double spaceWidth, double spaceHeight ) {
    const float width = sprite.GetSize().x, height = sprite.GetSize().y;
    const sf::FloatRect rect = sprite.GetImage()->GetTexCoords( sprite.GetSubRect() );
    int tx = (int)(0.5+(width-spaceWidth)/2.0), ty = (int)(0.5+(height-spaceHeight)/2.0);
    glTranslatef( -tx, -ty, 0 );
    using namespace std;
    glBegin( GL_QUADS );
    glTexCoord2f( rect.Left, rect.Top ); glVertex2f(0+0.5,0+0.5);
    glTexCoord2f( rect.Left, rect.Bottom ); glVertex2f(0+0.5,height+0.5);
    glTexCoord2f( rect.Right, rect.Bottom ); glVertex2f(width+0.5,height+0.5);
    glTexCoord2f( rect.Right, rect.Top ); glVertex2f(width+0.5,0+0.5);
    glEnd();
}
