#include "sftools.h"

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

