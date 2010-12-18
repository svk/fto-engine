#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>

int main(void) {
    int w = 800, h = 600;

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

    const double hr = 40.0;
    const double pi3 = M_PI / 3.0;
    sf::Shape hex;
    hex.AddPoint( hr * cos(pi3), hr * sin(pi3), sf::Color(255,255,255) );
    hex.AddPoint( hr, 0, sf::Color(255,255,255) );
    hex.AddPoint( hr * cos(-pi3), hr * sin(-pi3), sf::Color(255,255,255) );
    hex.AddPoint( hr * cos(pi3-M_PI), hr * sin(pi3-M_PI), sf::Color(255,255,255) );
    hex.AddPoint( -hr, 0, sf::Color(255,255,255) );
    hex.AddPoint( hr * cos(M_PI-pi3), hr * sin(M_PI-pi3), sf::Color(255,255,255) );
    hex.EnableFill( true );
    hex.EnableOutline( false );

    double r = 10;
    double x = w/2.0, y = h/2.0;
    double vm = 200, va = M_PI * 0.8;

    const sf::Input& input = win.GetInput();

    bool scrolling = false;
    double scrollingFromX, scrollingFromY;

    while( win.IsOpened() ) {
        const double dt = clock.GetElapsedTime();
        clock.Reset();

        sf::Event ev;
        while( win.GetEvent( ev ) ) {
            if( ev.Type == sf::Event::Resized ) {
                std::cerr << "we were resized! " << ev.Size.Width << " " << ev.Size.Height << std::endl;
                win.SetView( sf::View( sf::FloatRect( 0, 0, ev.Size.Width, ev.Size.Height ) ) );
                w = ev.Size.Width;
                h = ev.Size.Height;
                x = w/2;
                y = h/2;
            }

            if( ev.Type == sf::Event::Closed ) {
                win.Close();
            }

            if( ev.Type == sf::Event::MouseButtonPressed ) {
                double mx = input.GetMouseX();
                double my = input.GetMouseY();
                scrolling = true;
                scrollingFromX = mx;
                scrollingFromY = my;
                std::cerr << mx << " " << my << std::endl;
            }

            if( ev.Type == sf::Event::MouseButtonReleased ) {
                scrolling = false;
            }

            if( ev.Type == sf::Event::MouseMoved ) {
                if( scrolling ) {
                    double mx = ev.MouseMove.X;
                    double my = ev.MouseMove.Y;
                    kitten.Move( mx - scrollingFromX, my - scrollingFromY );
                    scrollingFromX = mx;
                    scrollingFromY = my;
                    std::cerr << "<" <<  mx << " " << my << std::endl;
                }
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

        helloWorld.SetCenter( clock.GetElapsedTime() * 10, -300 );

        win.Draw( kitten );
        win.Draw( helloWorld );

        hex.SetColor( sf::Color( 0, 128, 0, 128 ) );
        hex.SetPosition( x, y );
        win.Draw( hex );

        hex.SetColor( sf::Color( 128, 0, 0, 128 ) );
        hex.SetPosition( x + 1.5 * hr, y + hr * sin(pi3) );
        win.Draw( hex );
        hex.SetColor( sf::Color( 0, 0, 128, 128 ) );
        hex.SetPosition( x - 1.5 * hr, y + hr * sin(pi3) );
        win.Draw( hex );
        hex.SetColor( sf::Color( 128, 128, 0, 128 ) );
        hex.SetPosition( x + 1.5 * hr, y - hr * sin(pi3) );
        win.Draw( hex );
        hex.SetColor( sf::Color( 0, 128, 128, 128 ) );
        hex.SetPosition( x - 1.5 * hr, y - hr * sin(pi3) );
        win.Draw( hex );
        hex.SetColor( sf::Color( 128, 0, 128, 128 ) );
        hex.SetPosition( x, y + 2 * hr * sin(pi3) );
        win.Draw( hex );
        hex.SetColor( sf::Color( 0, 0, 0, 128 ) );
        hex.SetPosition( x, y - 2 * hr * sin(pi3) );
        win.Draw( hex );

        win.Draw( sf::Shape::Circle(x,y,r,sf::Color(255,0,0)) );

        win.Display();
    }

    return 0;
}
