#include <alice/cachalot/library/cachalot.h>
#include <alice/cachalot/library/main_class.h>

#include <voicetech/library/evlogdump/evlogdump.h>
#include <library/cpp/getopt/small/modchooser.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>


int RunWithoutDefaultConfig(int argc, const char** argv) {
    return NCachalot::NApplication::Run(argc, argv);
}

int main(int argc, const char** argv) {
    static const TVector<TCachalotMainClass> configs = NCachalot::ExecuteConfigOptions;

    TModChooser modChooser;

    for (const auto& cfg : configs) {
        modChooser.AddMode(cfg.GetModeName(), cfg, cfg.GetHelpString());
    }

    modChooser.AddMode("run", RunWithoutDefaultConfig, "Run cachalot without default config");
    modChooser.AddMode("evlogdump", EventLogDumpMain, "Run evlogdump");
    modChooser.SetDefaultMode("run");
    return modChooser.Run(argc, argv);
}
