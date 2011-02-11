#include "TacRules.h"

#include <iostream>

int main(int argc, char *argv[]) {
    using namespace std;
    Tac::OpposedBooleanRoller toHitRoll ( 0.75, 0.06 );
    for(int i=-100;i<=100;i++) {
        double c = toHitRoll.hitChance( i, 0 ).get_d();
        cout << i << " " << c << endl;
    }
    return 0;
}
