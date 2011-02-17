#include "SProto.h"

#include "NashServer.h"
#include "TacServer.h"

#include <csignal>

#include <sys/time.h>

#include "TacDungeon.h"

bool keepRunning = true;

void signal_stop(int sig) {
    using namespace std;
    cerr << "Caught signal " << sig << endl;
    keepRunning = false;
}

int main(int argc, char *argv[]) {
    using namespace std;

    SProto::Server server;

    Nash::NashSubserver ssNash ( server );

    using namespace Tac;
    MTRand_int32 prng ( time(0) );
    SimpleLevelGenerator *levelgen = 0;
    while( !levelgen ) {
        try {
            levelgen = new SimpleLevelGenerator( prng );
            levelgen->setRoomTarget( 5 );
            levelgen->setShortestCorridorsFirst();
            levelgen->setStopWhenConnected();
            levelgen->setSWCExtraCorridors( 2 );

            levelgen->adoptPainter( new Tac::HollowHexagonRoomPainter( 6, 2 ),
                                   1,
                                   false );
            levelgen->adoptPainter( new Tac::HollowHexagonRoomPainter( 7, 3 ),
                                   1,
                                   false );
            levelgen->adoptPainter( new Tac::BlankRoomPainter( 3 ),
                                   1,
                                   false );

            levelgen->adoptPainter( new Tac::HexagonRoomPainter( 4 ),
                                   2,
                                   true );
            levelgen->adoptPainter( new Tac::HexagonRoomPainter( 5 ),
                                   2,
                                   true );
            levelgen->adoptPainter( new Tac::HexagonRoomPainter( 6 ),
                                   2,
                                   true );
            levelgen->adoptPainter( new Tac::HexagonRoomPainter( 7 ),
                                   1,
                                   true );

            levelgen->generate();
        }
        catch( LevelGenerationFailure& e ) {
            using namespace std;
            cerr << "warning: level generation failure, trying again.. (" << e.what() << ")" << endl;
            delete levelgen;
            levelgen = 0;
        }
    }
    Tac::TacTestServer ssTacTest ( server, "./config/unit-types.lisp", "./config/tile-types.lisp", time(0), levelgen->getSketch() );
    delete levelgen;

    server.addListener( SPROTO_STANDARD_PORT );

    signal( SIGINT, signal_stop );
    signal( SIGTERM, signal_stop );

    cerr << "Server running. Listening on " << SPROTO_STANDARD_PORT << "." << endl;

    while( server.isRunning() && keepRunning ) {
        Timer timer;
        server.manage( 100 );
        server.tick( timer.getElapsedTime() );
    }

    cerr << "Server terminating. Writing persistent data..";

    server.save();

    cerr << "..done." << endl;
}
