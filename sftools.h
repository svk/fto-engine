#ifndef H_SFTOOLS
#define H_SFTOOLS

#include <SFML/Graphics.hpp>

#include <string>
#include <map>

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

template<class T>
class ResourceManager {
    // not thread-safe for assignment
    private:
        typedef std::map<std::string,T*> ResourceContainer;
        ResourceContainer resources;
    
    public:
        ResourceManager(void) {}
        ~ResourceManager(void) {
            for(typename ResourceContainer::iterator i=resources.begin();i != resources.end(); i++) {
                delete i->second;
            }
        }

        void bind(const std::string& str, T* resource) {
            typename ResourceContainer::iterator i = resources.find( str );
            if( i != resources.end() ) {
                delete i->second;
            }
            resources[ str ] = resource;
        }

        T& operator[](const std::string& str) {
            return *resources[ str ];
        }
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

#endif
