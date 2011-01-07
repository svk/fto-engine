#ifndef H_SFTOOLS
#define H_SFTOOLS

#include <SFML/Graphics.hpp>

#include <string>
#include <map>

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

class SfmlApplication;

class SfmlScreen : public SfmlEventHandler {
    private:
        SfmlApplication& app;

    public:
        virtual void draw(sf::RenderWindow&) = 0;
        virtual bool handleEvent(const sf::Event&) = 0;
        virtual void resize(int,int) = 0;
};

class SfmlApplication : public SfmlEventHandler {
    private:
        int width, height;

        sf::Clock clock;
        sf::RenderWindow window;
        sf::View mainView; // work out role
        SfmlScreen *currentScreen;

        void processIteration(void);
        void resize(int,int);

    public:
        SfmlApplication(std::string, int, int);
        ~SfmlApplication();

        void run(void);

        virtual void tick(double) {};

        bool handleEvent(const sf::Event&);

        void setScreen(SfmlScreen*);
};

#endif
