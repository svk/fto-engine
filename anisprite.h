#ifndef H_ANISPRITE
#define H_ANISPRITE

#include <vector>

#include <SFML/Graphics.hpp>

#include "typesetter.h"

#include "sftools.h"

class Animation {
    public:
        virtual void animate(double) = 0;
};

class AnimatedSprite : public sf::Drawable,
                       public Animation {
    private:
        typedef std::vector<sf::Sprite*> SpriteList;
        SpriteList sprites;
        double spriteDuration;
        bool doLoop;

        SpriteList::iterator currentSprite;
        double currentSpriteTTL;

    public:
        AnimatedSprite(double,bool);
        ~AnimatedSprite(void);

        void animate(double);
        void addImage(const sf::Image&);

        virtual void Render(sf::RenderTarget& target) const;
        void SetColor(const sf::Color&);
};

class RisingTextAnimation : public sf::Drawable,
                            public Animation,
                            public FiniteLifetimeObject {
    private:
        LabelSprite* labelSprite; // owned
        const double duration;

        const double x;
        double y;
        const double yspeed;
        double alpha;
        const double alphaspeed;

    public:
        RisingTextAnimation(double, double, LabelSprite*, double, double, int);
        RisingTextAnimation(double, double, LabelSprite*, double, double);
        ~RisingTextAnimation(void);

        void animate(double);
        virtual void Render(sf::RenderTarget& target) const;

        bool done(void) const;
};

#endif
