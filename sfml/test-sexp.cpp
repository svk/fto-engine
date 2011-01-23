#include "Sise.h"

#include <iostream>

int main(int argc, char *argv[]) {
    using namespace Sise;
    using namespace std;
    SExpStreamParser sp;
    std::string data;
    SExp *testy = readSExpFromFile( "./data/motd.lisp" );
    outputSExp( testy, cout );
    delete testy;
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
            writeSExpToFile( "./data/motd.lisp", rv );
            delete rv;
        }
    }
    return 0;
}
