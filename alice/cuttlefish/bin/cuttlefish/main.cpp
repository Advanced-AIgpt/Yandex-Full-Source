#include <alice/cuttlefish/library/cuttlefish/config/config.h>
#include <alice/cuttlefish/library/cuttlefish/run.h>

#include <alice/cuttlefish/library/logging/dlog.h>

#include <voicetech/library/evlogdump/evlogdump.h>

#include <library/cpp/getopt/small/modchooser.h>

using namespace NAlice::NCuttlefish;

int CuttlefishRun(int argc, const char** argv) {
    return Run(ParseCuttlefishConfigFromArgs(argc, argv));
}

int main(int argc, const char** argv) {
    DLOG("!!! ATTENTION: cuttlefish debug logs is enabled. This built should not be used in production");
    TModChooser modChooser;
    modChooser.AddMode("run", CuttlefishRun, "Run cuttlefish");
    modChooser.AddMode("evlogdump", EventLogDumpMain, "Run evlogdump");
    modChooser.SetDefaultMode("run");
    return modChooser.Run(argc, argv);
}
