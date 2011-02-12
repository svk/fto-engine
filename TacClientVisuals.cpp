#include "TacClientVisuals.h"

#include <cmath>

#include <sstream>

#include "sftools.h"

#include <SFML/Graphics.hpp>

#include <iostream>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

namespace Tac {

CosTable::CosTable(const int sectors) :
    sectors ( sectors ),
    value ( new double[sectors] )
{
    for(int i=0;i<sectors;i++) {
        const double t = 2 * M_PI * (((double)i)/(double)sectors);
        value[i] = cos(t);
    }
}

SinTable::SinTable(const int sectors) :
    sectors ( sectors ),
    value ( new double[sectors] )
{
    for(int i=0;i<sectors;i++) {
        const double t = 2 * M_PI * (((double)i)/(double)sectors);
        value[i] = sin(t);
    }
}

HpIndicator::HpIndicator(FreetypeFace& font, int hp, int maxHp) :
    font ( font ),
    hp ( hp ),
    maxHp ( maxHp ),
    text ( 0 )
{
    setState( hp, maxHp );
}

void HpIndicator::setState(int hp_, int maxHp_) {
    std::ostringstream oss;
    hp = hp_;
    maxHp = maxHp_;
    oss << hp;
    if( text ) {
        delete text;
    }
    text = new LabelSprite( oss.str(), sf::Color(0,0,0), font );
}

HpIndicator::~HpIndicator(void) {
    if( text ) {
        delete text;
    }
}

void HpIndicator::drawGL(int tileWidth, int tileHeight) {
    // assume centered on unit, pixel scale
    int sx = (int)(0.5 + (double(tileWidth))/2.0 + 3),
        sy = (int)(0.5 + (double(tileHeight))/2.0 + 3);
    const int r = 14;
    glPushMatrix();
        glTranslatef( sx, sy, 0 );
        glScalef( r, r, r );
        drawHpIndicatorGL( hp, maxHp );
    glPopMatrix();

    if( text ) {
        glPushMatrix();
        glTranslatef( sx, sy, 0 );
        glTranslatef( -r, -r, 0 );
        glTranslatef( 0, -3, 0 ); // hax (adjusting so text looks nice is complex wrt baseline etc)
        const float width = text->getSprite().GetSize().x, height = text->getSprite().GetSize().y;
        int tx = (int)(0.5+(width-2*r)/2.0), ty = (int)(0.5+(height-2*r)/2.0);
        glTranslatef( -tx, -ty, 0 );

        text->getSprite().GetImage()->Bind();

        glBegin( GL_QUADS );
        glTexCoord2f( 0, 0 ); glVertex2f(0,0);
        glTexCoord2f( 0, 1 ); glVertex2f(0,height);
        glTexCoord2f( 1, 1 ); glVertex2f(width,height);
        glTexCoord2f( 1, 0 ); glVertex2f(width,0);
        glEnd();
        glPopMatrix();
    }
}

void drawHpIndicatorGL(int hp,int maxHp) {
    // an alternative way of doing this: having two sprites, pasting part of one and part of the other
    // together
    const static int sectors = 100;
    const static SinTable sint (sectors);
    const static CosTable cost(sectors);

    if( hp < 0 ) hp = 0;

    const double ratio = (double)hp / (double) maxHp;
    const double miny = - (2 * ratio - 1);

    const float healthyr = 0 + 0.25, healthyg = 0.5 + 0.25, healthyb = 0.0 + 0.25;
    const float damagedr = 0.5 + 0.25, damagedg = 0.0 + 0.25, damagedb = 0.0 + 0.25;

    using namespace std;

    glDisable( GL_TEXTURE_2D );
    glEnable( GL_LINE_SMOOTH );

    if( hp < maxHp ) {
        glColor3f( damagedr, damagedg, damagedb );
        glBegin( GL_TRIANGLE_FAN );
        glVertex2f( 0, 0 );
        for(int i=0;i<sectors;i++) {
            glVertex2f( cost.value[i], sint.value[i] );
        }
        glVertex2f( cost.value[0], sint.value[0] );
        glEnd();
    }

    glColor3f( healthyr, healthyg, healthyb );
    glBegin( GL_TRIANGLE_FAN );
    glVertex2f( 0, 1 );
    for(int i=0;i<sectors;i++) {
        glVertex2f( cost.value[i], MAX(sint.value[i],miny) );
    }
    glVertex2f( cost.value[0], MAX(sint.value[0], miny) );
    glEnd();

    glColor3f( 0.0, 0.0, 0.0 );
    glBegin( GL_LINE_LOOP );
    for(int i=0;i<sectors;i++) {
        glVertex2f( cost.value[i], sint.value[i] );
    }
    glEnd();

}

PanelCenteredText::PanelCenteredText(FreetypeFace& font) :
    font ( font ),
    sprite ( 0 )
{
    set( "", sf::Color(0,0,0) );
}

PanelCenteredText::PanelCenteredText(FreetypeFace& font, const std::string& text_, const sf::Color& colour_) : 
    font ( font ),
    sprite ( 0 )
{
    set( text_, colour_ );
}

PanelCenteredText::~PanelCenteredText(void) {
    if( sprite ) {
        delete sprite;
    }
}

void PanelCenteredText::setColour(const sf::Color& colour_) {
    set( text, colour_ );
}

void PanelCenteredText::setText(const std::string& text_) {
    set( text_, colour );
}

void PanelCenteredText::set(const std::string& text_, const sf::Color& colour_) {
    if( sprite ) {
        delete sprite;
        sprite = 0;
    }
    text = text_;
    colour = colour_;
    sprite = new LabelSprite( text, colour, font );
}

int PanelCenteredText::getHeight(void) {
    return sprite->getHeight();
}

void PanelCenteredText::draw(sf::RenderWindow& window, int width) {
    sprite->restrictToWidth( width );
    int pad = (width - sprite->getWidth())/2;
    if( pad < 0 ) pad = 0;
    sprite->setPosition( pad, 0 );
    sprite->draw( window );
}

Panel::Panel(void) :
    x ( x ),
    y ( y )
{
}

void Panel::setPosition(int x_, int y_) {
    x = x_;
    y = y_;
}

void Panel::draw(sf::RenderWindow& window, int width) {
    sf::View myView( sf::FloatRect( -x, -y, - x + window.GetWidth(), - y + window.GetHeight() ) );
    window.SetView( myView );
    for(std::vector<PanelElement*>::iterator i = elements.begin(); i != elements.end(); i++) {
        (*i)->draw( window, width );
        myView.Move( 0, - (*i)->getHeight() );
    }
    window.SetView( window.GetDefaultView() );
}

}
