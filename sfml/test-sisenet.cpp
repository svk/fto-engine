#include "SiSeNet.h"

#include <iostream>
#include <ostream>

#include <string>

#include <cstring>
#include <cstdio>

struct LabelledSocket;

class Broadcaster {
    private:
        std::vector<LabelledSocket*>& list;

    public:
        Broadcaster( std::vector<LabelledSocket*>& list ) :
            list ( list )
        {
        }

        void sendChatMessage(const std::string& username, const std::string& message);
};

struct LabelledSocket : public SiSeNet::ConsSocket {
    std::string name;
    Broadcaster& broadcaster;

    LabelledSocket(std::string name, SiSeNet::RawSocket r, Broadcaster& broadcaster ) :
        SiSeNet::ConsSocket( r ),
        name ( name ),
        broadcaster( broadcaster )
    {
    }

    ~LabelledSocket(void) {
    }

    void handle( const std::string& event,  SiSExp::SExp* data ) {
        if( event == "chat-message" ) {
            broadcaster.sendChatMessage( name, data->asString()->get() );
        } else if( event == "nick-change" ) {
            name = data->asString()->get();
        }
    }
};

void Broadcaster::sendChatMessage(const std::string& username, const std::string& message) {
    using namespace SiSExp;
    Cons *bc = new Cons( new Symbol( "chat-message" ),
                         new Cons( new String( username ),
                                   new Cons( new String( message ) ) ) );
    for(std::vector<LabelledSocket*>::iterator i = list.begin(); i != list.end(); i++) {
        outputSExp( bc, (*i)->out() );
    }
    delete bc;
}



struct MyGreeter : public SiSeNet::SocketGreeter {
    private:
        std::vector<LabelledSocket*>& list;
        Broadcaster broadcaster;

    public:
        MyGreeter(std::vector<LabelledSocket*>& list) : list ( list ), broadcaster( list )  {}

        SiSeNet::Socket* greet(SiSeNet::RawSocket rs, struct sockaddr_storage* foo, socklen_t bar) {
            char buffer[1024];
            snprintf( buffer, sizeof buffer, "user%d", list.size() + 1 );
            LabelledSocket *rv = new LabelledSocket( buffer, rs, broadcaster );
            list.push_back( rv );
            return rv;
        }
};

int main(int argc, char *argv[]) {
    using namespace std;
    using namespace SiSeNet;
    using namespace SiSExp;
    const int port = 1337;

    ConsSocketManager man;
    if( argc == 1 ) {
        vector<LabelledSocket*> clients;
        MyGreeter greeter ( clients );

        man.addListener( port );
        man.setGreeter( &greeter );
        while( true ) {
            man.manage( 10000 );
#if 0
            for(SocketManager::SocketSet::iterator i = my.begin(); i != my.end(); i++) {
                std::string name = dynamic_cast<LabelledSocket*>( *i )->name;
                while( !(*i)->in().empty() ) {
                    Cons *bc = new Cons( new String( name ), new Cons( (*i)->in().pop() ) );
                    for(vector<LabelledSocket*>::iterator j = clients.begin(); j != clients.end(); j++) {
                        outputSExp( bc, (*j)->out() );
                    }
                    delete bc;
                }
            }
#endif
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
                std::string name = rv->asCons()->getcdr()->asCons()->getcar()->asString()->get();
                std::string message = rv->asCons()->getcdr()->asCons()->getcdr()->asCons()->getcar()->asString()->get();
                cout << "[" << name << "] " << message << endl;
                delete rv;
            }
        }
    }

    return 0;
}
