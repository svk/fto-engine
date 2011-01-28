#include "sftools.h"

#include <iostream>

class MyScreen : public SfmlScreen {
    private:
        sf::Shape myRect;
        sf::Color myColour;
    public:
        MyScreen(void) :
            myColour ( 255, 0, 0 )
        {
        }

        void draw(sf::RenderWindow& window) {
            window.Clear( sf::Color(200,0,0) );
            window.Draw( myRect );
        }

        bool handleEvent(const sf::Event& ev) {
            switch( ev.Type ) {
                case sf::Event::KeyPressed:
                    switch( ev.Key.Code ) {
                        case sf::Key::A:
                            myColour = sf::Color(255,0,0);
                            break;
                        case sf::Key::B:
                            myColour = sf::Color(0,200,100);
                            break;
                        default:
                            break;
                    }
                    myRect.SetColor( myColour );
                    return true;
                default:
                    return false;
            }
        }

        void resize(int width, int height) {
            myRect = sf::Shape::Rectangle( width - 110,
                                           height - 110,
                                           width - 10,
                                           height - 10,
                                           myColour );
        }
};

class MyExample : public SfmlApplication {
    public:
        MyExample(void) :
            SfmlApplication( "MyExample", 640, 480 )
        {
        }
};

int main(int argc, char *argv[]) {
    MyExample app;
    MyScreen mainScreen;
    app.setScreen( &mainScreen );
    app.run();
    return 0;
}
