#include <alice/cuttlefish/library/asr/adapter/service.h>
#include <alice/cuttlefish/library/asr/adapter/unistat.h>
#include "alice/cuttlefish/library/service_runner/service_runner.h"

using namespace NAlice;

int main(int argc, const char** argv) {
    NAsrAdapter::Unistat();
    return RunService<NAsrAdapter::TService>(argc, argv);
}
