#ifndef H_TAC_CLIENT_VISUALS
#define H_TAC_CLIENT_VISUALS

#include "typesetter.h"

namespace Tac {

struct CosTable {
    const int sectors;
    double *value;

    CosTable(const int sectors);
};

struct SinTable {
    const int sectors;
    double *value;

    SinTable(const int sectors);
};

void drawHpIndicatorGL(int,int);

class HpIndicator {
    private:
        FreetypeFace& font;
        int hp, maxHp;
        LabelSprite *text;

    public:
        HpIndicator(FreetypeFace&, int, int);
        ~HpIndicator(void);
        void setState(int,int);
        
        void drawGL(int,int);
};

};

#endif
