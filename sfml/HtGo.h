#ifndef H_HTGO
#define H_HTGO

#include "hexfml.h"

struct HtGoTile {
    int coreX, coreY;
    std::string positionalLabel;
};

class HexTorusGoMap {
    private:
        int radius;
        HexMap<HtGoTile> coreMap;

        HtGoTile& getIJR(int,int,int);
    public:
        HexTorusGoMap(int);
        HtGoTile& get(int,int);
        void debugLabelCore(void);
};

#endif
