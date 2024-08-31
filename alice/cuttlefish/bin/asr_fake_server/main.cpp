#include "alice/cuttlefish/library/asr/base/service.h"
#include <alice/cuttlefish/library/proto_configs/asr_fake_server.cfgproto.pb.h>
#include "alice/cuttlefish/library/service_runner/service_runner.h"

using namespace NAlice;

class TFakeService : public NAsr::TService {
public:
    typedef NAliceAsrFakeServerConfig::Config TConfig;
    TFakeService(const TConfig&)
        : NAsr::TService()
    {}
    static const TString DefaultConfigResource;
};

const TString TFakeService::DefaultConfigResource = "/asr_fake_server/default_config.json";

int main(int argc, const char** argv) {
    return RunService<TFakeService>(argc, argv);
}
