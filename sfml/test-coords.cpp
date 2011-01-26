#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cassert>

#include "hexfml.h"

int main(int argc, char *argv[]) {
    using namespace std;
    for(int i=0;i<200000;i++) {
        int x, y;
        inflateHexCoordinate(i, x, y);
        int ci, cj, cr;
        int x2, y2,i2;
        polariseHexCoordinate(x, y, ci, cj, cr);
        cartesianiseHexCoordinate(ci,cj,cr,x2,y2);
        i2 = flattenHexCoordinate(x,y);
        cout << x << " " << y << " ";
        cout << "[" << ci << "," << cj << "," << cr << "] ";
        cout << i2 << endl;
        assert( i == i2 );
        assert( x == x2 && y == y2 );
    }

}
