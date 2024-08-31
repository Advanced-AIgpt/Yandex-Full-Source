/*
    Proto+Jinja2 -> Universal Compiler
    Documentation and command line options: see ../readme.md

    Return codes:
        0 - success
        1 - error in proto files compilation
        2 - protobuf message class name not found
        3 - error opening destination file for writing
        4 - unknown error (exception)
*/

#include "jinja2_compiler.h"

#include <util/stream/output.h>
#include <util/generic/yexception.h>

//
// Application entry point
//
int main(int argc, const char** argv) {
    try {
        NAlice::TJinja2Compiler compiler;
        if (!compiler.InitOptions(argc, argv)) {
            compiler.PrintUsage();
            return 5;
        }
        return compiler.Run();
    } catch (...) {
        Cerr << "Unhandled exception occured" << Endl;
        Cerr << FormatCurrentException() << Endl;
    }
    return 4;
}
