#include <alice/cuttlefish/library/tts/cache/proxy/service.h>
#include <alice/cuttlefish/library/service_runner/service_runner.h>

using namespace NAlice;

int main(int argc, const char** argv) {
    return RunService<NTtsCacheProxy::TService>(argc, argv);
}
