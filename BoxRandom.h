#ifndef H_BOX_RANDOM
#define H_BOX_RANDOM

// no relation to box-müller

#include <vector>

#include <gmpxx.h>

#include <iostream>

struct GmpRandom {
        static GmpRandom global;

        gmp_randstate_t randstate;

        GmpRandom(void);
        GmpRandom(unsigned long int);
        ~GmpRandom(void);
};

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

        int getNumberOfOutcomes(void) const { return outcomes.size(); }
        const N& getTotalWeight(void) const { return sum; }
        const N& getWeight(int j) const { return weights[j]; }
        O getOutcome(int j) const { return outcomes[j]; }
};

template
<class I, class O, class N = mpq_class>
class NondeterministicTransform {
    public:
        virtual Outcomes<O,N> transform(I) = 0;

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
        virtual O transform(I) = 0;

        Outcomes<O,N> operator()(const Outcomes<I,N>& orig) {
            const int sz = orig.getNumberOfOutcomes();
            Outcomes<O,N> rv;
            for(int i=0;i<sz;i++) {
                rv.add( orig.getWeight(i), transform(orig.getOutcome(i)) );
            }
            return rv;
        }
};

template
<class O>
O chooseRandomOutcome(const Outcomes<O,mpq_class>& outcomes, gmp_randclass& prng) {
    // this is NOT optimized for a huge amount of usage
    // but it IS supposed to be accurate
    const int sz = outcomes.getNumberOfOutcomes();
    mpz_class hugeval (1);
    for(int i=0;i<sz;i++) {
        hugeval *= outcomes.getWeight(i).get_den();
    }
    using namespace std;
    mpq_class r ( prng.get_z_range( hugeval * outcomes.getTotalWeight() ), hugeval );
    for(int i=0;i<sz;i++) {
        r -= outcomes.getWeight(i);
        if( r < 0 ) {
            return outcomes.getOutcome(i);
        }
    }
    throw std::logic_error( "chooseRandomOutcome is borked" );
}

#endif
