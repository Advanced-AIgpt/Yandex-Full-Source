#include <library/cpp/testing/benchmark/bench.h>
#include <alice/cuttlefish/library/evcheck/evcheck.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <util/folder/path.h>
#include <util/stream/file.h>

namespace {

NVoice::TParser PARSER = NVoice::ConstructSynchronizeStateParser();

const TFsPath DATA_ROOT = TFsPath(ArcadiaSourceRoot()) / "alice/cuttlefish/library/evcheck/bench";

const TString USUAL_SYNCHRONIZE_STATE = TFileInput(DATA_ROOT/"ss_usual.txt").ReadAll();

}

Y_CPU_BENCHMARK(ParseUsual, iface) {
    for (size_t i = 0; i < iface.Iterations(); ++i) {
        PARSER.ParseJson(USUAL_SYNCHRONIZE_STATE);
    }
}
