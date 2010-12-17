#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>

int main(void) {
    const int w = 800, h = 600;

    sf::RenderWindow win ( sf::VideoMode(800,600,32),
                           "521 SFML test" );
    sf::Font font;
    sf::Clock clock;
    sf::Image kittenImage;

    if( !font.LoadFromFile( "data/Vera.ttf" ) ) {
        std::cerr << "fatal error: unable to load font" << std::endl;
        return 1;
    }
    
    if( !kittenImage.LoadFromFile( "data/kitten.jpeg" ) ) {
        std::cerr << "fatal error: failed to find kitten" << std::endl;
        return 1;
    }

    sf::String helloWorld ("Hello world", font, 30 );
    sf::Sprite kitten;
    kitten.SetImage( kittenImage );

    double r = 10;
    double x = w/2.0, y = h/2.0;
    double vm = 200, va = M_PI * 0.8;

    while( win.IsOpened() ) {
        const double dt = clock.GetElapsedTime();
        clock.Reset();

        sf::Event ev;
        while( win.GetEvent( ev ) ) {
            if( ev.Type == sf::Event::Closed ) {
                win.Close();
            }
        }

        double xp = dt * vm * cos(va) + x,
               yp = dt * vm * sin(va) + y;
        bool fx = (xp - r < 0) || (xp + r >= w),
             fy = (yp - r < 0) || (yp + r >= h);
        if( fx ) {
            va = M_PI - va;
        }
        if( fy ) {
            va = -va;
        }
        x += dt * vm * cos(va);
        y += dt * vm * sin(va);

        win.Clear(sf::Color(200,0,200));

        win.Draw( kitten );
        win.Draw( helloWorld );

        win.Draw( sf::Shape::Circle(x,y,r,sf::Color(255,0,0)) );

        helloWorld.SetCenter( clock.GetElapsedTime() * 10, -300 );

        win.Display();
    }

    return 0;
}
