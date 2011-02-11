#include "TacRules.h"

#include <iostream>

int main(int argc, char *argv[]) {
    using namespace std;
    using namespace Tac;

    for(int i=-100;i<=100;i++) {
        Outcomes<AttackResult> results = makeAttack(i,0);
        results = DamageDealer(i,0,8,4)( results );
        const int sz = results.getNumberOfOutcomes();
        mpq_class total = 0;
        for(int j=0;j<sz;j++) {
            total += results.getWeight(j) * getDamageOfAttack( results.getOutcome(j) );
        }
        total /= results.getTotalWeight();
        double expval = total.get_d();
        cout << i << " " << expval << endl;
    }
    return 0;
}
