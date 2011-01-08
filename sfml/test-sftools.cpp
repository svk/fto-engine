#include "sftools.h"

#include <iostream>

class MyScreen : public SfmlScreen {
    private:
        sf::Shape myRect;
    public:
        void draw(sf::RenderWindow& window) {
            window.Clear( sf::Color(200,0,0) );
            window.Draw( myRect );
        }

        bool handleEvent(const sf::Event& ev) {
        }

        void resize(int width, int height) {
            myRect = sf::Shape::Rectangle( width - 110,
                                           height - 110,
                                           width - 10,
                                           height - 10,
                                           sf::Color( 100, 100, 100 ) );

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
