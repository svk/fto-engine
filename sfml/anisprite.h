#ifndef H_ANISPRITE
#define H_ANISPRITE

#include <vector>

#include <SFML/Graphics.hpp>

class AnimatedSprite : public sf::Drawable {
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

#endif
