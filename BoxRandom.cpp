#include "BoxRandom.h"

#include <time.h>

GmpRandom GmpRandom::global;

GmpRandom::GmpRandom(void) {
    gmp_randinit_mt( randstate );
    gmp_randseed_ui( randstate, time(0) + clock() );
}

GmpRandom::GmpRandom(unsigned long int seed) {
    gmp_randinit_mt( randstate );
    gmp_randseed_ui( randstate, seed );
}

GmpRandom::~GmpRandom(void) {
    gmp_randclear( randstate );
}
