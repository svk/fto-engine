#include "Sise.h"

#include <iostream>

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>

class TestHandler : public Sise::NamedSexpHandler {
    public:
        void handleNamedSExp(const std::string& name, Sise::SExp *sexp) {
            using namespace Sise;
            using namespace std;
            cout << name << ": ";
            outputSExp( sexp, cout );
            cout << endl;
        }
};

int main(int argc, char *argv[]) {
    using namespace Sise;
    using namespace std;
    SExpStreamParser sp;
    TestHandler handler;
    std::string data;
    readSExpDir( "./data/", ".lisp", handler );
    while( true ) {
        getline( cin, data );
        for(int i=0;i<(int)data.length();i++) {
            sp.feed( data[i] );
        }
        sp.end();
        while( !sp.empty() ) {
            SExp *rv = sp.pop();
            if( rv ) {
                rv->output( cout );
            } else {
                cout << "()";
            }
            cout << endl;
            if( !writeSExpToFile( "./data/motd.lisp", rv ) ) {
                cerr << "unsuccessful write!" << endl;
            }
            delete rv;
        }
    }
    return 0;
}
