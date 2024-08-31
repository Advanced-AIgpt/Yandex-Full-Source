#include <alice/cuttlefish/library/proto_configs/yabio_fake_server.cfgproto.pb.h>
#include <alice/cuttlefish/library/service_runner/service_runner.h>
#include <alice/cuttlefish/library/yabio/base/service.h>

using namespace NAlice;

class TFakeService : public NAlice::NYabio::TService {
public:
    typedef NAliceYabioFakeServerConfig::Config TConfig;
    TFakeService(const TConfig&)
        : NAlice::NYabio::TService()
    {}
    static const TString DefaultConfigResource;
};

const TString TFakeService::DefaultConfigResource = "/yabio_fake_server/default_config.json";

int main(int argc, const char** argv) {
    return RunService<TFakeService>(argc, argv);
}
