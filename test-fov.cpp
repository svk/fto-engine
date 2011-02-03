#include "HexFov.h"

#include <iostream>

class DummyOpacity : public HexTools::HexOpacityMap {
    public:
        bool isOpaque(int x,int y) const {
            if( x == 0 && y == 10 ) return true;
            if((x*x+y*y) > 40*40) return true;
            return false;
        }
};

class DummyReceiver : public HexTools::HexLightReceiver {
    public:
        void setLit(int x, int y) {
            using namespace std;
            cout << x << " " << y << endl;
        }
};

int main(int argc, char *argv[]) {
    using namespace HexTools;
    DummyOpacity opac;
    DummyReceiver rec;
    for(int i=0;i<6;i++) {
        HexFovBeam beam ( opac, rec, i, -3, 9 );
        beam.calculate();
    }
    return 0;
}
