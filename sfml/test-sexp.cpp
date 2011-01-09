#include "SiSexp.h"

int main(int argc, char *argv[]) {
    SiSExp::SExp *value = new SiSExp::Int(1337);
    delete value;
    return 0;
}
