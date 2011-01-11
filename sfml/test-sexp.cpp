#include "SiSExp.h"

#include <iostream>

int main(int argc, char *argv[]) {
    using namespace SiSExp;
    using namespace std;
    SExpStreamParser sp;
    std::string data;
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
            delete rv;
        }
    }
    return 0;
}
