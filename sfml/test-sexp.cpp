#include "SiSExp.h"

#include <iostream>

int main(int argc, char *argv[]) {
    using namespace SiSExp;
    using namespace std;
    Cons *list = 0;
    list = new Cons( new Int(1337), list );
    list = new Cons( new Int(1336), list );
    list = new Cons( new Int(1335), list );
    list = new Cons( new String("numbers sorted in ascending order:"), list );
    list->output( cout );
    return 0;
}
