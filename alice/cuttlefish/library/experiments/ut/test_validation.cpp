#include <alice/cuttlefish/library/experiments/experiments.h>
#include <alice/cuttlefish/library/cuttlefish/common/utils.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include "common.h"

using namespace NVoice::NExperiments;

namespace {

const TFsPath TEST_DATA = TFsPath(ArcadiaSourceRoot()) / "alice/cuttlefish/library/experiments/ut/data";
const TFsPath PROD_DATA = TFsPath(ArcadiaSourceRoot()) / "alice/uniproxy/experiments";

TVector<NJson::TJsonValue> GetEvents(const TFsPath& path)
{
    TVector<NJson::TJsonValue> jsons;
    TFileInput fin(path);
    TString line;
    while (fin.ReadLine(line) > 0) {
        NJson::TJsonValue json;
        NJson::ReadJsonTree(line, &json, true);
        jsons.emplace_back(std::move(json));
    }
    return jsons;
}

} // anonymous namespace


Y_UNIT_TEST_SUITE(ValidationTests) {


Y_UNIT_TEST(ValidateProductionExperiments) {
    // ensure that production experiments are accepted
    UNIT_ASSERT_NO_EXCEPTION(
        TExperiments(TExperiments::TConfig{PROD_DATA/"experiments_rtc_production.json", PROD_DATA/"vins_experiments.json"})
    );
    UNIT_ASSERT_NO_EXCEPTION(
        TExperiments(TExperiments::TConfig{PROD_DATA/"experiments_ycloud.json", PROD_DATA/"vins_experiments.json"})
    );
}


Y_UNIT_TEST(ValidateWithoutUaaS) {
    TVector<NJson::TJsonValue> synchronizeStates = GetEvents(TEST_DATA/"synchronize_states.jsons");
    const TVector<NJson::TJsonValue> sampleEvents = GetEvents(TEST_DATA/"sample_events.jsons");
    const TVector<NJson::TJsonValue> pachedEvents = GetEvents(TEST_DATA/"patched_events.jsons");
    UNIT_ASSERT_EQUAL(synchronizeStates.size() * (1 + sampleEvents.size()), pachedEvents.size());  // self check

    const TExperiments experiments(TExperiments::TConfig{TEST_DATA/"experiments.json", TEST_DATA/"macros.json"});

    auto patchedIt = pachedEvents.begin();
    for (NJson::TJsonValue& ss : synchronizeStates) {
        NAliceProtocol::TSessionContext sessionContext = CreateSessionContext(ss);
        const TEventPatcher patcher = experiments.CreatePatcherForSession(sessionContext, ss);

        patcher.Patch(ss, sessionContext);
        // Cerr << TJsonAsPretty{ss} << Endl;
        // UNIT_ASSERT_EQUAL(ss, *patchedIt);
        ++patchedIt;

        // for (const NJson::TJsonValue& i : sampleEvents) {
        //     NJson::TJsonValue event(i);
        //     UNIT_ASSERT(patcher(event, sessionContext));
        //     UNIT_ASSERT_EQUAL(event, *patchedIt);
        //     ++patchedIt;
        // }
    }
}

};  // Y_UNIT_TEST_SUITE(ValidationTests)
