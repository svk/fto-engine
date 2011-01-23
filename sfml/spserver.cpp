#include "SProto.h"

int main(int argc, char *argv[]) {
    SProto::Server server;
    server.addListener( 8990 );
    while( true ) {
        server.manage( 1000 );
    }
}
