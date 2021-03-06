#ifndef H_SFTOOLS
#define H_SFTOOLS

#include <stdexcept>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include <string>
#include <map>

#include <vector>

#include "Manager.h"

class FiniteLifetimeObject {
    public:
        virtual ~FiniteLifetimeObject(void) {}

        virtual bool done(void) const = 0;
};

template
<class T>
struct FiniteLifetimeObjectList {
    typedef std::vector<T*> List;
    List objects;

    ~FiniteLifetimeObjectList(void) {
        while( !objects.empty() ) {
            delete objects.back();
            objects.pop_back();
        }
    }

    void prune(void) {
        typename List::iterator i = objects.begin();
        while( i != objects.end() ) {
            if( (*i)->done() ) {
                delete *i;
                i = objects.erase( i );
            } else {
                i++;
            }
        }
    }

    void adopt(T* arg) {
        objects.push_back( arg );
    }
};

class PixelTransform {
    public:
        virtual sf::Color transform(const sf::Color&) = 0;
        sf::Image* apply(sf::Image*);
};

class ToTranslucent : public PixelTransform {
    private:
        int alpha;
    public:
        ToTranslucent(int alpha) : alpha(alpha) {}
        sf::Color transform(const sf::Color&);
};

class ToGrayscale : public PixelTransform {
    public:
        sf::Color transform(const sf::Color&);
};

class SfmlEventHandler {
    private:
        SfmlEventHandler *next;
    
    protected:
        bool delegate(const sf::Event&);
        
    public:
        SfmlEventHandler(void);

        void setNextEventHandler(SfmlEventHandler*);
        virtual bool handleEvent(const sf::Event&) = 0;
};

class CompositeEventHandler : public SfmlEventHandler {
    // For a whole bunch of event handlers where
    // the internal ordering doesn't matter or
    // will never change.
    private:
        // owned pointers
        typedef std::vector<SfmlEventHandler*> HandlerList;
        HandlerList handlers;

    public:
        CompositeEventHandler(void);
        ~CompositeEventHandler(void);

        void adoptHandler(SfmlEventHandler*);

        bool handleEvent(const sf::Event&);
};

class SfmlMouseTarget {
    public:
        virtual bool isAt(int, int) const = 0;
};

class SfmlApplication;

class SfmlScreen : public SfmlEventHandler {
    public:
        virtual void draw(sf::RenderWindow&) = 0;
        virtual bool handleEvent(const sf::Event&) = 0;
        virtual void resize(int,int) = 0;
};

class SfmlApplication : public SfmlEventHandler {
    private:
        int width, height;

        sf::Clock clock;
        sf::View mainView;
        SfmlScreen *currentScreen;

        bool keepRunning;

        void processIteration(void);
        void resize(int,int);

        sf::RenderWindow window;

    public:
        SfmlApplication(std::string, int, int);
        ~SfmlApplication();

        sf::View& getMainView(void) { return mainView; }

        sf::RenderWindow& getWindow(void) { return window; }

        void run(void);
        void stop(void);

        virtual void tick(double) {};

        bool handleEvent(const sf::Event&);

        void setScreen(SfmlScreen*);
};

class SpritesheetFull {
};

class Spritesheet {
    private:
        sf::Image sheet;
        int width, height;

        int x, y;
        int rowHeight;

        std::vector<sf::IntRect> rects;

    public:
        Spritesheet(int,int);
        virtual ~Spritesheet(void) {}

        int adopt(sf::Image*);
        bool hasSpaceFor(int,int) const;

        sf::Sprite makeSprite(int) const;
        sf::FloatRect getSpriteRect(int) const;

        void bindTexture(void);
};

template<typename T>
class SimpleKeyedSpritesheet : public Spritesheet {
    private:
        std::map<T,int> keys;
    public:
        SimpleKeyedSpritesheet(int w, int h) :
            Spritesheet ( w , h ),
            keys () 
        {
        }

        void adopt(const T& str, sf::Image* img) {
            int rv = Spritesheet::adopt( img );
            keys[str] = rv;
        }

        sf::Sprite makeSprite(const T& key) const {
            typename std::map< T, int >::const_iterator i = keys.find( key );
            if( i == keys.end() ) {
                throw std::logic_error( "no such sprite key" );
            }
            return Spritesheet::makeSprite( i->second );
        }

        sf::FloatRect getSpriteRect(const T& key) const {
            typename std::map< T, int >::const_iterator i = keys.find( key );
            return Spritesheet::getSpriteRect( i->second );
        }
};

typedef SimpleKeyedSpritesheet<std::string> StringKeyedSpritesheet;

sf::SoundBuffer* loadSoundBufferFromFile(const std::string&);
sf::Image* loadImageFromFile(const std::string&);

void drawSprite( const sf::Sprite& sprite );
void drawSpriteCentered( const sf::Sprite&, double, double);
void drawBoundSprite( const sf::Sprite& sprite );
void drawBoundSpriteCentered( const sf::Sprite&, double, double);

#endif
