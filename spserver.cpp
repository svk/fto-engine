#include "SProto.h"

#include "NashServer.h"

#include <csignal>

bool keepRunning = true;

void signal_stop(int sig) {
    using namespace std;
    cerr << "caught ctrl-c" << endl;
    keepRunning = false;
}

int main(int argc, char *argv[]) {
    using namespace std;

    SProto::Server server;

    Nash::NashSubserver ssNash ( server );

    server.addListener( SPROTO_STANDARD_PORT );

    signal( SIGINT, signal_stop );
    signal( SIGTERM, signal_stop );

    cerr << "Server running. Listening on " << SPROTO_STANDARD_PORT << "." << endl;

    while( server.isRunning() && keepRunning ) {
        server.manage( 1000 );
    }

    cerr << "Server terminating. Writing persistent data..";

    server.save();

    cerr << "..done." << endl;
}