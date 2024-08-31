#include <alice/cuttlefish/library/experiments/experiments.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <util/folder/path.h>
#include <library/cpp/json/json_writer.h>
#include "common.h"

using namespace NVoice::NExperiments;

const TFsPath TEST_DATA = TFsPath(ArcadiaSourceRoot()) / "alice/cuttlefish/library/experiments/ut/data";

Y_UNIT_TEST_SUITE(SharingExperiments) {

Y_UNIT_TEST(IndependentSharingExperiments)
{
    const TString experimentsFilename = MakeFile(R"__([
        {
            "id": "ONE",
            "share": 0.4,
            "flags": [
                ["set", ".Sharing-04", 1]
            ]
        }, {
            "id": "TWO",
            "share": 0.7,
            "flags": [
                ["set", ".Sharing-07", 1]
            ]
        }
    ])__");
    TExperiments experiments(TExperiments::TConfig{experimentsFilename, TEST_DATA/"macros.json"});

    {
        const NJson::TJsonValue initialEvent = CreateJson(R"__({
            "header": {},
            "payload": {"uuid": "baaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"}
        })__");

        // random values should be: ~0.83, ~0.42
        const TEventPatcher eventPatcher = experiments.CreatePatcherForSession(
            CreateSessionContext(initialEvent), initialEvent
        );

        NJson::TJsonValue event = CreateJson(R"__({"header":{}, "payload": {}})__");
        eventPatcher.Patch(event, {});
        UNIT_ASSERT_VALUES_EQUAL(event, CreateJson(R"__({"header":{}, "payload": {
            "request": {"experiments":{}},
            "Sharing-07": 1
        }})__"));
    }

    {
        const NJson::TJsonValue initialEvent = CreateJson(R"__({
            "header": {},
            "payload": {"uuid": "daaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"}
        })__");

        // random values should be: ~0.38, ~0.30
        const TEventPatcher eventPatcher = experiments.CreatePatcherForSession(
            CreateSessionContext(initialEvent), initialEvent
        );

        NJson::TJsonValue event = CreateJson(R"__({"header":{}, "payload": {}})__");
        eventPatcher.Patch(event, {});
        UNIT_ASSERT_VALUES_EQUAL(event, CreateJson(R"({"header":{}, "payload": {
            "request":{"experiments":{}},
            "Sharing-04": 1,
            "Sharing-07": 1
        }})"));
    }
}

};
