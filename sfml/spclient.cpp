#include <iostream>

#include <boost/program_options.hpp>

#include "SProto.h"

class OutputClientCore : public SProto::ClientCore {
    public:
        void handle( const std::string& name, Sise::SExp* arg ) {
            using namespace std;
            cout << "[remote:" << name << "] ";
            Sise::outputSExp( arg, cout );
        }
};

int main(int argc, char *argv[]) {
    using namespace std;
    namespace po = boost::program_options;

    po::positional_options_description pd;

    pd.add( "host", 1 )
      .add( "port", 1 );

    po::options_description desc( "Allowed options" );
    desc.add_options()
        ("help", "display option help")
        ("username", po::value<string>(), "set username")
        ("password", po::value<string>(), "set password")
        ("host", po::value<string>(), "server host name or IP")
        ("port", po::value<int>()->default_value( SPROTO_STANDARD_PORT ), "server port")
        ;
    po::variables_map vm;
    po::store( po::command_line_parser( argc, argv ).options( desc ).positional( pd ).run(), vm );
    po::notify( vm );

    if( vm.count( "help" ) ) {
        cout << desc << endl;
        return 1;
    }

    SProto::Client *clientSocket = Sise::connectToAs<SProto::Client>( vm["host"].as<string>(), vm["port"].as<int>() );
    Sise::SocketManager manager;
    Sise::SExpStreamParser parser;
    OutputClientCore core;
    clientSocket->setCore( &core );

    manager.adopt( clientSocket );
    manager.checkStdin();

    if( vm.count( "username" ) && vm.count( "password" ) ) {
        clientSocket->identify( vm["username"].as<string>(), vm["password"].as<string>() );
    } else if( vm.count( "username" ) || vm.count( "password" ) ) {
        cerr << "warning: both a username and a password must be supplied to log in" << endl;
    }

    while( manager.numberOfWatchedSockets() > 0 ) {
        manager.pump( 1000, 0 );
        if( manager.stdinFlagged() ) {
            string in;
            getline( cin, in );
            for(int i=0;i<(int)in.length();i++) {
                parser.feed( in[i] );
            }
        }
        while( !parser.empty() ) {
            Sise::SExp *sexp = parser.pop();
            outputSExp( sexp, clientSocket->out() );
            delete sexp;
        }
        clientSocket->pump();
    }


    return 0;
}
