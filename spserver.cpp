#include "SProto.h"

#include "NashServer.h"
#include "TacServer.h"

#include <csignal>

#include <sys/time.h>

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
    Tac::TacTestServer ssTacTest ( server, 10 );

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
