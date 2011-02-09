#include "BoxRandom.h"

#include <iostream>

struct MaybeAdd10IfOddTransform : public NondeterministicTransform<int,int> {
    Outcomes<int> transform(const int& x) {
        Outcomes<int> rv;
        if( (x%2) == 0 ) {
            rv.add( 1, x );
        } else {
            rv.add( 1, x );
            rv.add( 1, 10 + x );
        }
        return rv;
    }
};

struct TripleTransform : public DeterministicTransform<int,int> {
    int transform(const int& x) {
        return 3 * x;
    }
};

struct HalveTransform : public DeterministicTransform<int,int> {
    int transform(const int& x) {
        return x/2;
    }
};

int main(int argc, char *argv[]) {
    using namespace std;
    Outcomes<int> outcomes;
    for(int i=1;i<=6;i++) {
        outcomes.add( 1, i );
    }
    outcomes = HalveTransform()( outcomes );
    outcomes = TripleTransform()( outcomes );
    outcomes = MaybeAdd10IfOddTransform()( outcomes );
    for(int i=0;i<outcomes.getNumberOfOutcomes();i++) {
        mpq_class p = outcomes.getWeight(i) / outcomes.getTotalWeight();
        cout << p << ": " << outcomes.getOutcome(i) << endl;
    }
    return 0;
}
