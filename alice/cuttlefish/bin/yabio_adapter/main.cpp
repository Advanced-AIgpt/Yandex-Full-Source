#include <alice/cuttlefish/library/service_runner/service_runner.h>
#include <alice/cuttlefish/library/yabio/adapter/service.h>
#include <alice/cuttlefish/library/yabio/adapter/unistat.h>

using namespace NAlice;

int main(int argc, const char** argv) {
    NYabioAdapter::Unistat();
    return RunService<NYabioAdapter::TService>(argc, argv);
}
