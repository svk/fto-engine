#ifndef H_TAC_CLIENT_VISUALS
#define H_TAC_CLIENT_VISUALS

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include "typesetter.h"

namespace Tac {

class PanelElement {
    public:
        virtual int getHeight(void) = 0;
        virtual void draw(sf::RenderWindow&, int) = 0;
};

struct PanelSpacer : public PanelElement {
    int height;
    PanelSpacer(int height) : height(height) {}
    int getHeight(void) { return height; }
    void draw(sf::RenderWindow&, int) {}
};

struct PanelCenteredText : public PanelElement {
    FreetypeFace& font;

    std::string text;
    sf::Color colour;
    LabelSprite *sprite;

    PanelCenteredText(FreetypeFace&);
    PanelCenteredText(FreetypeFace&, const std::string&, const sf::Color&);
    ~PanelCenteredText(void);

    void setText(const std::string&);
    void setColour(const sf::Color&);
    void set(const std::string&, const sf::Color&);

    int getHeight(void);

    void draw(sf::RenderWindow&, int);
};

class Panel {
    private:
        int x, y;
        std::vector<PanelElement*> elements;

    public:
        Panel(void);

        void setPosition(int x, int y);
        void clear(void) { elements.clear(); }
        void add(PanelElement* el) { // does NOT adopt
            elements.push_back( el );
        }

        void draw(sf::RenderWindow&, int);
};

struct CosTable {
    const int sectors;
    double *value;

    CosTable(const int sectors);
};

struct SinTable {
    const int sectors;
    double *value;

    SinTable(const int sectors);
};

void drawHpIndicatorGL(int,int);

class HpIndicator {
    private:
        FreetypeFace& font;
        int hp, maxHp;
        LabelSprite *text;

    public:
        HpIndicator(FreetypeFace&, int, int);
        ~HpIndicator(void);
        void setState(int,int);
        
        void drawGL(int,int);
};

void drawUnitSocketGL(int,int,sf::Color,int);
void drawUnitSocketGL(int,int,int,int,int,int);

};

#endif
