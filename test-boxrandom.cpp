#include "BoxRandom.h"

#include <iostream>

struct MaybeAdd10IfOddTransform : public NondeterministicTransform<int,int> {
    Outcomes<int> transform(int x) {
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
    int transform(int x) {
        return 3 * x;
    }
};

struct HalveTransform : public DeterministicTransform<int,int> {
    int transform(int x) {
        return x/2;
    }
};

struct AddBernoulliTrial : public NondeterministicTransform<int,int> {
    mpq_class successP;

    AddBernoulliTrial(mpq_class successP) : successP( successP ) {}

    Outcomes<int> transform(int x) {
        Outcomes<int> rv;
        rv.add( successP, x + 1 );
        rv.add( mpq_class(1) - successP, x );
        return rv;
    }
};

int main(int argc, char *argv[]) {
    using namespace std;
    Outcomes<int> outcomes;
    outcomes.add( 1, 0 );
    for(int i=0;i<20;i++) {
        outcomes = AddBernoulliTrial(mpq_class(1,2))( outcomes );
    }
    outcomes = HalveTransform()( outcomes );
    outcomes = TripleTransform()( outcomes );
    outcomes = MaybeAdd10IfOddTransform()( outcomes );
    mpq_class exval = 0;
    for(int i=0;i<outcomes.getNumberOfOutcomes();i++) {
        mpq_class p = outcomes.getWeight(i) / outcomes.getTotalWeight();
        exval += outcomes.getOutcome(i) * outcomes.getWeight(i);
        cout << p << ": " << outcomes.getOutcome(i) << endl;
    }
    exval /= outcomes.getTotalWeight();

    cout << endl;

    cout << "Expected value: " << exval << endl;
    cout << "                " << exval.get_d() << endl;

    gmp_randclass prng (gmp_randinit_mt);

    const int trials1 = 100000;
    double s = 0;
    for(int i=0;i<trials1;i++) {
        s += chooseRandomOutcome( outcomes, prng );
    }
    s /= trials1;
    cout << "Average value:  " << s << endl;

    cout << endl;

    Outcomes<bool> outcomes2;
    outcomes2.add( mpq_class(49,100), true );
    outcomes2.add( mpq_class(51,100), false );
    const int trials2 = 100000;
    int successes = 0;
    for(int i=0;i<trials2;i++) {
        if( chooseRandomOutcome( outcomes2, prng ) ) {
            ++successes;
        }
    }
    cout << "E(successes): 49%" << endl;
    cout << "Successes: " << successes << "/" << trials2 << endl;
    return 0;
}
