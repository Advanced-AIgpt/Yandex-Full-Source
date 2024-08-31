#include <alice/cuttlefish/library/tts/backend/base/service.h>
#include <alice/cuttlefish/library/proto_configs/tts_fake_server.cfgproto.pb.h>

#include <alice/cuttlefish/library/service_runner/service_runner.h>

class TFakeService : public NAlice::NTts::TService {
public:
    typedef NAliceTtsFakeServerConfig::Config TConfig;
    explicit TFakeService(const TConfig&)
        : NAlice::NTts::TService()
    {}
    static const TString DefaultConfigResource;
};

const TString TFakeService::DefaultConfigResource = "/tts_fake_server/default_config.json";

int main(int argc, const char** argv) {
    return NAlice::RunService<TFakeService>(argc, argv);
}
