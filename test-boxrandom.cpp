#include "BoxRandom.h"

#include <iostream>

#include <gmpxx.h>

void maybeAdd10IfOdd(const int& x, const mpz_class& w, Outcomes<int,mpz_class>& o) {
    if( (x%2) == 0 ) {
        o.add( 2 * w, x );
    } else {
        o.add( w, x );
        o.add( w, 10 + x );
    }
}

int triple(const int& x) {
    return x*3;
}

int halve(const int& x) {
    return x/2;
}

int main(int argc, char *argv[]) {
    using namespace std;
    Outcomes<int,mpz_class> outcomes;
    for(int i=1;i<=6;i++) {
        outcomes.add( 1, i );
    }
    outcomes = transformDeterministic<int,int,mpz_class>( outcomes, halve );
    outcomes = transformDeterministic<int,int,mpz_class>( outcomes, triple );
    outcomes = transformNondeterministic<int,int,mpz_class>( outcomes, maybeAdd10IfOdd );
    for(int i=0;i<outcomes.getNumberOfOutcomes();i++) {
        mpq_class p (outcomes.getWeight(i), outcomes.getTotalWeight());
        cout << p << ": " << outcomes.getOutcome(i) << endl;
    }
    return 0;
}
