#include <alice/cuttlefish/library/experiments/experiment_patch.h>
#include <alice/cuttlefish/library/experiments/ut/common.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/json/json_reader.h>
#include <util/folder/path.h>

using namespace NVoice::NExperiments;


Y_UNIT_TEST_SUITE(PatchFunctions_IfSessionDataEq) {

Y_UNIT_TEST(IfSessionDataEq) {
    const NJson::TJsonValue initialEvent1 = CreateJson(R"__({
        "header": {},
        "payload": {"auth_token": "51ae06cc-5c8f-48dc-93ae-7214517679e6"}
    })__");

    const NJson::TJsonValue initialEvent2 = CreateJson(R"__({
        "header": {},
        "payload": {"auth_token": "ffffffff-ffff-ffff-ffff-ffffffffffff"}
    })__");

    const TExpPatch patch = CreateExperimentPatch(R"__([
        "if_session_data_eq", ".key", "51ae06cc-5c8f-48dc-93ae-7214517679e6"
    ])__");

    NJson::TJsonValue event = CreateJson(R"__({"header": {}, "payload": {}})__");

    UNIT_ASSERT(patch.Apply(event, CreateSessionContext(initialEvent1)));
    UNIT_ASSERT(!patch.Apply(event, CreateSessionContext(initialEvent2)));
}

}; // Y_UNIT_TEST_SUITE(PatchFunctions_IfSessionDataEq)
