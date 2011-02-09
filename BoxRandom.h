#ifndef H_BOX_RANDOM
#define H_BOX_RANDOM

// no relation to box-müller

#include <vector>

#include <gmpxx.h>

template
<class O, class N = mpq_class>
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
<class I, class O, class N = mpq_class>
class NondeterministicTransform {
    public:
        virtual Outcomes<O,N> transform(const I&) = 0;

        Outcomes<O,N> operator()(const Outcomes<I,N>& orig) {
            const int sz = orig.getNumberOfOutcomes();
            Outcomes<O,N> rv;
            for(int i=0;i<sz;i++) {
                const N& mwt = orig.getWeight(i);
                const Outcomes<O,N> mrv = transform( orig.getOutcome(i) );
                const int msz = mrv.getNumberOfOutcomes();
                const N& totw = mrv.getTotalWeight();
                for(int j=0;j<msz;j++) {
                    rv.add( mwt * mrv.getWeight(j) / totw, mrv.getOutcome(j) );
                }
            }
            return rv;
        }
};

template
<class I, class O, class N = mpq_class>
class DeterministicTransform {
    public:
        virtual O transform(const I&) = 0;

        Outcomes<O,N> operator()(const Outcomes<I,N>& orig) {
            const int sz = orig.getNumberOfOutcomes();
            Outcomes<O,N> rv;
            for(int i=0;i<sz;i++) {
                rv.add( orig.getWeight(i), transform(orig.getOutcome(i)) );
            }
            return rv;
        }
};

#endif
