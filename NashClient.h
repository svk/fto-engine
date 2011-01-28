#ifndef H_NASHCLIENT
#define H_NASHCLIENT

#include "Nash.h"
#include "sftools.h"

namespace Nash {

class NashBlitter : public HexBlitter {
    private:
        NashBoard& board;
        ResourceManager<HexSprite>& hsman;

        bool selected;
        int selectedX, selectedY;

    public:
        NashBlitter(NashBoard& board, ResourceManager<HexSprite>& hsman) :
            board ( board ),
            hsman ( hsman ),
            selected ( false )
        {
        }

        void drawHex(int, int, sf::RenderWindow&);

        void setSelected(int,int);
        void setNoSelected(void);
};

}

#endif
