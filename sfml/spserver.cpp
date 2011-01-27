#include "SProto.h"

#include "NashServer.h"

int main(int argc, char *argv[]) {
    SProto::Server server;

    Nash::NashSubserver ssNash ( server );

    server.addListener( SPROTO_STANDARD_PORT );

    while( server.isRunning() ) {
        server.manage( 1000 );
    }
}
