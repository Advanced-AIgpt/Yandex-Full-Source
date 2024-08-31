#include "config.h"

#include <library/cpp/proto_config/load.h>
#include <library/cpp/resource/resource.h>

namespace NAlice::NCuttlefish {

namespace {

static TString DEFAULT_CUTTLEFISH_CONFIG_RESOURCE = "/cuttlefish/proto_config/config.json";

}  // anonymous namespace

NAliceCuttlefishConfig::TConfig GetCuttlefishConfigFromResource(const TString& resource) {
    NProtoConfig::TLoadConfigOptions cfgOpt;
    cfgOpt.Resource = resource;

    return NProtoConfig::LoadWithOptions<NAliceCuttlefishConfig::Config>(cfgOpt);
}

NAliceCuttlefishConfig::TConfig GetDefaultCuttlefishConfig() {
    return GetCuttlefishConfigFromResource(DEFAULT_CUTTLEFISH_CONFIG_RESOURCE);
}

NAliceCuttlefishConfig::TConfig ParseCuttlefishConfigFromArgs(int argc, const char** argv) {
    NProtoConfig::TLoadConfigOptions cfgOpt;
    cfgOpt.Resource = DEFAULT_CUTTLEFISH_CONFIG_RESOURCE;

    return NProtoConfig::GetOpt<NAliceCuttlefishConfig::Config>(argc, argv, DEFAULT_CUTTLEFISH_CONFIG_RESOURCE);
}

}  // namespace NAlice::NCuttlefish
