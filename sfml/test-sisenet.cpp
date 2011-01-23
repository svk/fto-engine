#include "Sise.h"

#include <iostream>
#include <ostream>
#include <algorithm>

#include <string>

#include <cstring>
#include <cstdio>

struct LabelledSocket : public Sise::ConsSocket {
    std::string netname;
    std::string name;
    std::vector<LabelledSocket*>& list;

    LabelledSocket(std::string name, Sise::RawSocket r, std::vector<LabelledSocket*>& list ) :
        Sise::ConsSocket( r ),
        netname( name ),
        name ( name ),
        list ( list )
    {
        using namespace std;
        list.push_back( this );
        cerr << "[connect] " << netname << " as " << this << endl;
    }

    ~LabelledSocket(void) {
        using namespace std;
        list.erase( remove( list.begin(), list.end(), this ), list.end() );
        cerr << "[disconnect] " << netname << " as " << this << endl;
    }

    void handle( const std::string& event,  Sise::SExp* data ) {
        if( event == "chat-message" ) {
            sendChatMessage( name, asString(data)->get() );
        } else if( event == "nick-change" ) {
            name = asString(data)->get();
        }
    }

    void sendChatMessage(const std::string& username, const std::string& message) {
        using namespace Sise;
        SExp *bc = List()( new Symbol( "chat-message" ) )
                         ( new String( username ) )
                         ( new String( message ) )
                   .make();
        for(std::vector<LabelledSocket*>::iterator i = list.begin(); i != list.end(); i++) {
            outputSExp( bc, (*i)->out() );
        }
        delete bc;
    }
};


struct MyGreeter : public Sise::SocketGreeter {
    private:
        std::vector<LabelledSocket*>& list;

    public:
        MyGreeter(std::vector<LabelledSocket*>& list) : list ( list ) {}

        Sise::Socket* greet(Sise::RawSocket rs, struct sockaddr_storage* foo, socklen_t bar) {
            std::ostringstream oss;
            oss << Sise::getAddressString( foo, bar ) << ":" << Sise::getPort( foo, bar );
            LabelledSocket *rv = new LabelledSocket( oss.str(), rs, list );
            return rv;
        }
};

int main(int argc, char *argv[]) {
    using namespace std;
    using namespace Sise;
    const int port = 1337;

    ConsSocketManager man;
    if( argc == 1 ) {
        vector<LabelledSocket*> clients;
        MyGreeter greeter ( clients );

        man.addListener( port );
        man.setGreeter( &greeter );
        while( true ) {
            man.manage( 10000 );
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
                SExp *out;
                if( in.length() > 0 && in[0] == '/' ) {
                    out = new Cons( new Symbol( "nick-change" ),
                                    new String( in.substr(1) ) );
                } else {
                    out = new Cons( new Symbol( "chat-message" ),
                                    new String( in ) );
                }
                outputSExp( out, conn->out() );
                delete out;
            }
            while( !conn->in().empty() ) {
                SExp *rv = conn->in().pop();
                std::string name = *asString( asProperCons(rv)->nthcar(1));
                std::string message = *asString( asProperCons(rv)->nthcar(2));
                cout << "[" << name << "] " << message << endl;
                delete rv;
            }
        }
    }

    return 0;
}
