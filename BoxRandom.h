#ifndef H_BOX_RANDOM
#define H_BOX_RANDOM

// no relation to box-müller

#include <vector>

template
<class O, class N = int>
class Outcomes {
    private:
        N sum;
        std::vector<N> weights;
        std::vector<O> outcomes;

    public:
        Outcomes(void) : sum(0), weights(), outcomes() {
        }

        void add(const N& w, const O& o) {
            int index = 0;
            sum += w;
            for(typename std::vector<O>::iterator i = outcomes.begin(); i != outcomes.end(); i++) {
                if( *i == o ) {
                    weights[index] += w;
                    return;
                }
                ++index;
            }
            weights.push_back( w );
            outcomes.push_back( o );
        }

        const O& choose(void) {
            return outcomes[0]; // chosen by fair dice roll etc etc ha ha
        }

        int getNumberOfOutcomes(void) const { return outcomes.size(); }
        const N& getTotalWeight(void) const { return sum; }
        const N& getWeight(int j) const { return weights[j]; }
        const O& getOutcome(int j) const { return outcomes[j]; }
};

template
<class I,class O,class N>
Outcomes<O,N> transformDeterministic(const Outcomes<I,N>& orig, O (*f)(const I&)) {
    const int sz = orig.getNumberOfOutcomes();
    Outcomes<O,N> rv;
    for(int i=0;i<sz;i++) {
        rv.add( orig.getWeight(i), f(orig.getOutcome(i)) );
    }
    return rv;
}

template
<class I,class O,class N>
Outcomes<O,N> transformNondeterministic(const Outcomes<I,N>& orig, void (*f)(const I&, const N&, Outcomes<O,N>&)) {
    const int sz = orig.getNumberOfOutcomes();
    Outcomes<O,N> rv;
    for(int i=0;i<sz;i++) {
        f( orig.getOutcome(i), orig.getWeight(i), rv );
    }
    return rv;
}

#endif
