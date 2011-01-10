#include "SiSeNet.h"

#include <iostream>
#include <ostream>

#include <string>

#include <cstring>
#include <cstdio>

struct LabelledSocket : public SiSeNet::Socket {
    std::string name;
    LabelledSocket(std::string name, SiSeNet::RawSocket r) :
        SiSeNet::Socket( r ),
        name ( name )
    {
    }
};

struct MyGreeter : public SiSeNet::SocketGreeter {
    private:
        int n;

    public:
        MyGreeter(void) : n ( 0 ) {}

        SiSeNet::Socket* greet(SiSeNet::RawSocket rs, struct sockaddr_storage* foo, socklen_t bar) {
            char buffer[1024];
            snprintf( buffer, sizeof buffer, "user%d", ++n );
            return new LabelledSocket( buffer, rs );
        }
};


int main(int argc, char *argv[]) {
    using namespace std;
    using namespace SiSeNet;
    using namespace SiSExp;
    const int port = 1337;
    MyGreeter greeter;

    SocketManager man;
    if( argc == 1 ) {
        man.addListener( port );
        man.setGreeter( &greeter );
        SocketManager::SocketSet my;
        while( true ) {
            my.clear();
            man.pump( 10000, &my );
            for(SocketManager::SocketSet::iterator i = my.begin(); i != my.end(); i++) {
                SocketManager::SocketSet all = man.debugGetSockets();
                std::string name = dynamic_cast<LabelledSocket*>( *i )->name;
                while( !(*i)->in().empty() ) {
                    Cons *bc = new Cons( new String( name ), new Cons( (*i)->in().pop() ) );
                    for(SocketManager::SocketSet::iterator j = all.begin(); j != all.end(); j++) {
                        outputSExp( bc, (*j)->out() );
                    }
                    delete bc;
                }
            }
        }
    } else {
        Socket *conn = Socket::connectTo( argv[1], port );
        man.adopt( conn );
        man.checkStdin();
        while( man.numberOfWatchedSockets() > 0 ) {
            man.pump( 10000, 0 );
            if( man.stdinFlagged() ) {
                string in;
                getline( cin, in );
                String *out = new String( in );
                outputSExp( out, conn->out() );
                delete out;
            }
            while( !conn->in().empty() ) {
                SExp *rv = conn->in().pop();
                cout << "[" << rv->asCons()->getcar()->asString()->get() << "] ";
                cout << rv->asCons()->getcdr()->asCons()->getcar()->asString()->get();
                cout << endl;
            }
        }
    }

    return 0;
}
