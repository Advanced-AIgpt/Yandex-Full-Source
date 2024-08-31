#include "app_config.h"

#include <library/cpp/getoptpb/getoptpb.h>


namespace NGProxy {

TApplicationConfig::TApplicationConfig(int argc, const char **argv) {
    NGetoptPb::TGetoptPbSettings getOptSettings;
    getOptSettings.PrettyOpts = true;
    getOptSettings.DontRequireRequired = false;
    getOptSettings.DumpConfig = true;

    TString errorMessage;
    if (!NGetoptPb::GetoptPb(argc, argv, Config_, errorMessage, getOptSettings)) {
        Cerr << errorMessage << Endl;
        IsBadConfig_ = true;
    }


    // GetOptPb does not show nested repeated messages, do it manually
    Cerr << Config_.GetAppHost().DebugString() << Endl;
    Cerr << "====================" << Endl;
}

}   // namespace NGProxy
