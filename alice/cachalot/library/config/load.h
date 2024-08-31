#pragma once

#include <alice/cachalot/library/config/application.cfgproto.pb.h>


namespace NCachalot::NConfig {

    NCachalot::TApplicationSettings LoadApplicationConfig(
        int argc, const char **argv,
        const TString& defaultConfigResource
    );

}  // namespace NCachalot::NConfig
