#include "SProto.h"

int main(int argc, char *argv[]) {
    SProto::Server server;
    server.addListener( SPROTO_STANDARD_PORT );
    while( server.isRunning() ) {
        server.manage( 1000 );
    }
}
