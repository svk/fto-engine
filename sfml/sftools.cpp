#include "sftools.h"

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

void SfmlApplication::run(void) {
    while( window.IsOpened() ) {
        processIteration();
    }
}

void CompositeEventHandler::adoptHandler(SfmlEventHandler* handler) {
    handlers.push_back( handler );
}

void SfmlApplication::resize(int width_, int height_) {
    width = width_;
    height = height_;
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
    window ( sf::VideoMode( width, height, 32 ), title ),
    currentScreen ( 0 )
{
    window.SetFramerateLimit( 60 );
    window.UseVerticalSync( true );
}

SfmlApplication::~SfmlApplication(void) {
}

