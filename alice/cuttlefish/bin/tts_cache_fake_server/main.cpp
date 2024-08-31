#include <alice/cuttlefish/library/tts/cache/base/service.h>
#include <alice/cuttlefish/library/proto_configs/tts_cache_fake_server.cfgproto.pb.h>

#include <alice/cuttlefish/library/service_runner/service_runner.h>

using namespace NAlice;

class TFakeService : public NAlice::NTtsCache::TService {
public:
    typedef NAliceTtsCacheFakeServerConfig::Config TConfig;
    TFakeService(const TConfig&)
        : NAlice::NTtsCache::TService()
    {}
    static const TString DefaultConfigResource;
};

const TString TFakeService::DefaultConfigResource = "/tts_cache_fake_server/default_config.json";

int main(int argc, const char** argv) {
    return RunService<TFakeService>(argc, argv);
}
