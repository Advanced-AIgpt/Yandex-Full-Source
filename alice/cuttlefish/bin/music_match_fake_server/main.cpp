#include <alice/cuttlefish/library/music_match/base/service.h>
#include <alice/cuttlefish/library/proto_configs/music_match_fake_server.cfgproto.pb.h>
#include <alice/cuttlefish/library/service_runner/service_runner.h>

using namespace NAlice;

class TFakeService : public NAlice::NMusicMatch::TService {
public:
    typedef NAliceMusicMatchFakeServerConfig::Config TConfig;
    TFakeService(const TConfig&)
        : NAlice::NMusicMatch::TService()
    {}
    static const TString DefaultConfigResource;
};

const TString TFakeService::DefaultConfigResource = "/music_match_fake_server/default_config.json";

int main(int argc, const char** argv) {
    return RunService<TFakeService>(argc, argv);
}
