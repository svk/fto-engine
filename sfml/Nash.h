#ifndef H_NASH
#define H_NASH

#include "hexfml.h"

namespace Nash {

struct NashTile {
    enum Status {
        FREE,
        PIECE,
        EDGE,
        OFF_MAP
    };

    enum Colour {
        NONE,
        WHITE,
        BLACK,
        WHITE_BLACK,
        BLACK_WHITE
    };

    Status status;
    Colour colour;
    
    NashTile(void) :
        status ( OFF_MAP ),
        colour ( NONE )
    {
    }
};

class NashBoard {
    private:
        int size;
        HexMap<NashTile> tiles;

    public:
        NashBoard(int);
        ~NashBoard(void);

        bool isLegalMove(int,int) const;
        std::string getAppearance(int,int) const;

        void put(int,int,NashTile::Colour);
};

}

#endif
