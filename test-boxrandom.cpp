#include "BoxRandom.h"

#include <iostream>

#include <gmpxx.h>

Outcomes<int,mpq_class> maybeAdd10IfOdd(const int& x) {
    Outcomes<int,mpq_class> rv;
    if( (x%2) == 0 ) {
        rv.add( 1, x );
    } else {
        rv.add( 1, x );
        rv.add( 1, 10 + x );
    }
    return rv;
}

int triple(const int& x) {
    return x*3;
}

int halve(const int& x) {
    return x/2;
}

int main(int argc, char *argv[]) {
    using namespace std;
    Outcomes<int,mpq_class> outcomes;
    for(int i=1;i<=6;i++) {
        outcomes.add( 1, i );
    }
    outcomes = transformDeterministic<int,int,mpq_class>( outcomes, halve );
    outcomes = transformDeterministic<int,int,mpq_class>( outcomes, triple );
    outcomes = transformNondeterministic<int,int,mpq_class>( outcomes, maybeAdd10IfOdd );
    for(int i=0;i<outcomes.getNumberOfOutcomes();i++) {
        mpq_class p = outcomes.getWeight(i) / outcomes.getTotalWeight();
        cout << p << ": " << outcomes.getOutcome(i) << endl;
    }
    return 0;
}
