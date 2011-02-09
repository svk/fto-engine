#include "BoxRandom.h"

#include <iostream>

void maybeAdd10IfOdd(const int& x, const int& w, Outcomes<int,int>& o) {
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
    Outcomes<int,int> outcomes;
    for(int i=1;i<=6;i++) {
        outcomes.add( 1, i );
    }
    outcomes = transformDeterministic<int,int,int>( outcomes, halve );
    outcomes = transformDeterministic<int,int,int>( outcomes, triple );
    outcomes = transformNondeterministic<int,int,int>( outcomes, maybeAdd10IfOdd );
    for(int i=0;i<outcomes.getNumberOfOutcomes();i++) {
        double p = outcomes.getWeight(i) / (double) outcomes.getTotalWeight();
        cout << p << ": " << outcomes.getOutcome(i) << endl;
    }
    return 0;
}
