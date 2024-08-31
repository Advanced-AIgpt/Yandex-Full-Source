#include <alice/cuttlefish/library/music_match/adapter/service.h>
#include <alice/cuttlefish/library/service_runner/service_runner.h>

using namespace NAlice;

int main(int argc, const char** argv) {
    NMusicMatchAdapter::Unistat();
    return RunService<NMusicMatchAdapter::TService>(argc, argv);
}
