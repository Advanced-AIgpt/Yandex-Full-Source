#include "debug_applet.h"
#include "parser.h"
#include "common.h"
#include <library/cpp/getopt/last_getopt.h>

int RunDebugApplet(int argc, const char** argv) {
    NLastGetopt::TOpts opts = CreateOptions();

    TString inputText;
    opts.AddLongOption("text", "Text to be parsed")
        .RequiredArgument("text")
        .Required()
        .StoreResult(&inputText);

    TConfigFst config;
    AddFstConfigOpts(opts, config);

    NLastGetopt::TOptsParseResult confingOpts(&opts, argc, argv);

    Cout << "[Input text]\t" << inputText << Endl;
    TParser parser(config);
    const TString parsedString = parser.Parse(inputText);
    Cout << '[' << config.FstName << "]\t" << parsedString << Endl;

    return 0;
}
