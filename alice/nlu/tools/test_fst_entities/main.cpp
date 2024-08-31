#include "common.h"
#include "dataset.h"
#include "parser.h"
#include "debug_applet.h"
#include "parse_applet.h"
#include <library/cpp/getopt/small/modchooser.h>

int main(int argc, const char** argv) {
    TModChooser modChooser;

    modChooser.AddMode("parse", RunParseApplet,
                       "Parse dataset.");

    modChooser.AddMode("debug", RunDebugApplet,
                       "Parse sample.");

    return modChooser.Run(argc, argv);
}
