#include <alice/cuttlefish/library/tts/backend/adapter/service.h>

#include <alice/cuttlefish/library/service_runner/service_runner.h>

int main(int argc, const char** argv) {
    return NAlice::RunService<NAlice::NTtsAdapter::TService>(argc, argv);
}
