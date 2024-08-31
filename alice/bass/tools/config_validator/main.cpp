#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/fetcher/request.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/stream/output.h>


int main(int argc, const char** argv) {
    NLastGetopt::TOpts opts;
    opts.SetFreeArgsNum(1);
    opts.SetFreeArgTitle(0, "<config.json>", "configuration file in json");
    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    TString configFile = res.GetFreeArgs()[0];
    ui16 httpPort = 12345;
    TConfig::TKeyValues vars;

    try {
        TConfig(TFileInput(configFile).ReadAll(), httpPort, vars);
    } catch (const yexception& e) {
        Cerr << e.what() << Endl;
        return 1;
    }

    return 0;
}
