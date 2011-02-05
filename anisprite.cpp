#include "anisprite.h"

#include <cstdio>
#include <cassert>

#include <iostream>

using namespace std;

AnimatedSprite::AnimatedSprite(double duration, bool doLoop) :
    sprites (),
    spriteDuration ( duration ),
    doLoop ( doLoop ),
    currentSprite ( ),
    currentSpriteTTL ( 0.0 )
{
}

AnimatedSprite::~AnimatedSprite(void) {
    for(SpriteList::iterator i = sprites.begin(); i != sprites.end(); i++) {
        delete *i;
    }
}

void AnimatedSprite::addImage(const sf::Image& img) {
    sf::Sprite *sprite = new sf::Sprite();
    sprite->SetImage( img );
    sprites.push_back( sprite );
    currentSprite = sprites.begin();
}

void AnimatedSprite::animate(double dt) {
    dt = fmod( dt, spriteDuration * sprites.size() );
    while( dt >= currentSpriteTTL ) {
        SpriteList::iterator next = currentSprite + 1;
        if( next == sprites.end() ) {
            if( !doLoop ) return;
            next = sprites.begin();
        }
        currentSprite = next;
        dt -= currentSpriteTTL;
        currentSpriteTTL = spriteDuration;
    }
    currentSpriteTTL -= dt;
}

void AnimatedSprite::SetColor( const sf::Color& col ) {
    // Workaround for colours not being inherited in SFML1
    for(SpriteList::iterator i = sprites.begin(); i != sprites.end(); i++) {
        (*i)->SetColor( col );
    }
}

void AnimatedSprite::Render(sf::RenderTarget& target) const {
    assert( currentSprite != sprites.end() );
    target.Draw( **currentSprite );
}

void RisingTextAnimation::animate(double dt) {
    y -= dt * yspeed;
    alpha -= dt * alphaspeed;
    if( alpha < 0 ) alpha = 0.0;
    labelSprite->setPosition( x, y );
    if( alpha < 255 ) {
        labelSprite->setAlpha( (int)(0.5+ alpha) );
    } else {
        labelSprite->setAlpha( 255 );
    }
}

bool RisingTextAnimation::done(void) const {
    return alpha <= 0.0;
}

void RisingTextAnimation::Render(sf::RenderTarget& target) const {
    labelSprite->draw( target );
}

RisingTextAnimation::RisingTextAnimation(double x, double y, LabelSprite *labelSprite, double duration, double yspeed, int alpha) :
    labelSprite ( labelSprite ),
    duration ( duration ),
    x ( x ),
    y ( y ),
    yspeed ( yspeed ),
    alpha ( alpha ),
    alphaspeed( ((double)alpha) / duration )
{
    labelSprite->setPosition( x, y );
}

RisingTextAnimation::RisingTextAnimation(double x, double y, LabelSprite *labelSprite, double duration, double yspeed) :
    labelSprite ( labelSprite ),
    duration ( duration ),
    x ( x ),
    y ( y ),
    yspeed ( yspeed ),
    alpha ( 255.0 ),
    alphaspeed( 255.0 / duration )
{
    labelSprite->setPosition( x, y );
}

RisingTextAnimation::~RisingTextAnimation(void) {
    delete labelSprite;
}
