#include "Angelic.h"

#include <string>
#include <iostream>

int testsFailed = 0;
int testsRun = 0;

void runTest( const std::string& desc, int loc, bool result ) {
    using namespace std;

    ++testsRun;
    if( !result ) {
        ++testsFailed;
        cerr << "[failed] " << desc << " at " << loc << endl;
    }
}

void summarizeTests(void) {
    using namespace std;
    if( testsFailed > 0 ) {
        cerr << "Failed. [" << testsFailed << "/" << testsRun << "]" << endl;
    } else {
        cerr << "All OK. Ran " << testsRun << " tests without failure." << endl;
    }
}

int main(int argc, char *argv[]) {
    using namespace Angelic;
    using namespace std;

    Coordinate p0 (1,0),
               p1 (3,1),
               p2 (1,3),
               p3 (-1,3),
               p4 (-3,1),
               p5 (-3,-1),
               p6 (-1, -3),
               p7 (1, -3),
               p8 (3, -1),
               p9 (9999999,-1);

    runTest( "greater-than", __LINE__, p1 > p0 );
    runTest( "greater-than", __LINE__, p2 > p1 );
    runTest( "greater-than", __LINE__, p3 > p2 );
    runTest( "greater-than", __LINE__, p4 > p3 );
    runTest( "greater-than", __LINE__, p5 > p4 );
    runTest( "greater-than", __LINE__, p6 > p5 );
    runTest( "greater-than", __LINE__, p7 > p6 );
    runTest( "greater-than", __LINE__, p8 > p7 );
    runTest( "greater-than", __LINE__, p9 > p8 );
    runTest( "less-than", __LINE__, p0 < p1 );
    runTest( "less-than", __LINE__, p1 < p2 );
    runTest( "less-than", __LINE__, p2 < p3 );
    runTest( "less-than", __LINE__, p3 < p4 );
    runTest( "less-than", __LINE__, p4 < p5 );
    runTest( "less-than", __LINE__, p5 < p6 );
    runTest( "less-than", __LINE__, p6 < p7 );
    runTest( "less-than", __LINE__, p7 < p8 );
    runTest( "less-than", __LINE__, p8 < p9 );

    summarizeTests();

    return 0;
}
