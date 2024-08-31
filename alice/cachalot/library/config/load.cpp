#include "load.h"

#include <library/cpp/proto_config/load.h>


namespace NCachalot::NConfig {

    void LoadApplicationConfigAsProto(
        int argc, const char **argv,
        const TString& defaultConfigResource,
        NCachalot::ApplicationSettings& settings
    ) {
        NProtoConfig::GetOpt(argc, argv, settings, defaultConfigResource);

        if (settings.GetServer().GetGrpcPort() == 0) {
            settings.MutableServer()->SetGrpcPort(
                settings.GetServer().GetPort() + 1
            );
        }

        settings.MutableStats()->SetEnabled(true);
    }


    NCachalot::TApplicationSettings LoadApplicationConfig(
        int argc, const char **argv,
        const TString& defaultConfigResource
    ) {
        NCachalot::ApplicationSettings appCfg;
        LoadApplicationConfigAsProto(argc, argv, defaultConfigResource, appCfg);
        return NCachalot::TApplicationSettings(appCfg);
    }

}  // namespace NCachalot::NConfig
