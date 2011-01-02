#include "hexfml.h"

#include <iostream>

int main(int argc, char *argv[]) {
    using namespace std;

    const int sx = atoi(argv[1]),
              sy = atoi(argv[2]);

    ScreenGrid grid ( "./data/hexproto1.png" );

    for(int row=0;row<24;row++) {
        for(int col=0;col<79;col++) {
            int x = col, y = row;
            grid.screenToHex( x, y, sx, sy );
            if( x == 0 && y == -2 ) {
                cout << 'x';
            } else {
                cout << ' ';
            }

        }
        cout << endl;
    }

    return 0;
}
