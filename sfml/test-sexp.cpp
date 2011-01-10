#include "SiSExp.h"

#include <iostream>

int main(int argc, char *argv[]) {
    using namespace SiSExp;
    using namespace std;
    SExpStreamParser sp;
    std::string data ("1 2 3 \"hello\" () (1 2 3) (8 9 10 . 11)" );
    for(int i=0;i<(int)data.length();i++) {
        sp.feed( data[i] );
    }
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
    return 0;
}
