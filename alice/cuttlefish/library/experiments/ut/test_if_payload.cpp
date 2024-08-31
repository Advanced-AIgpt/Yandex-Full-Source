#include <alice/cuttlefish/library/experiments/experiment_patch.h>
#include <alice/cuttlefish/library/experiments/session_context_proxy.h>
#include <alice/cuttlefish/library/experiments/ut/common.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/json/json_reader.h>
#include <util/folder/path.h>

using namespace NVoice::NExperiments;


Y_UNIT_TEST_SUITE(PatchFunctions_IfPayload) {


Y_UNIT_TEST(TIfPayloadEq_InvalidArgument) {
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "if_payload_eq" ])__"), yexception);
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "if_payload_eq", ".some.path" ])__"), yexception);
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "if_payload_eq", 1, "value" ])__"), yexception);
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "if_payload_eq", ["some", "path"], "value" ])__"), yexception);
}

Y_UNIT_TEST(TIfPayloadEqWithLang) {
    const TExpPatch patch = CreateExperimentPatch(R"__(["if_payload_eq", ".lang", "Y"])__");
    {
        auto event = CreateJson(R"__({"header": {}, "payload": {"lang": "Z"}})__");
        UNIT_ASSERT(!patch.Apply(event, {}));
    } {
        auto event = CreateJson(R"__({"header": {}, "payload": {"lang": "Y"}})__");
        UNIT_ASSERT(patch.Apply(event, {}));
    } {
        NAliceProtocol::TSessionContext sessionContext;
        SetLang(sessionContext, "Z");

        auto event = CreateJson(R"__({"header": {}, "payload": {}})__");
        UNIT_ASSERT(!patch.Apply(event, sessionContext));
    } {
        NAliceProtocol::TSessionContext sessionContext;
        SetLang(sessionContext, "Y");

        auto event = CreateJson(R"__({"header": {}, "payload": {}})__");
        UNIT_ASSERT(patch.Apply(event, sessionContext));
    }
}

Y_UNIT_TEST(TIfPayloadEqWithAppId) {
    const TExpPatch patch = CreateExperimentPatch(R"__([
        "if_payload_eq", ".vins.application.app_id", "A"
    ])__");
    {
        NAliceProtocol::TSessionContext sessionContext;
        SetAppId(sessionContext, "A");
        auto event = CreateJson(R"__({"header": {}, "payload": {
            "vins": {"application": {"app_id": "B"}}
        }})__");
        UNIT_ASSERT(!patch.Apply(event, sessionContext));
    } {
        auto event = CreateJson(R"__({"header": {}, "payload": {
            "vins": {"application": {"app_id": "A"}}
        }})__");
        UNIT_ASSERT(patch.Apply(event, {}));
    } {
        NAliceProtocol::TSessionContext sessionContext;
        SetAppId(sessionContext, "Z");
        auto event = CreateJson(R"__({"header": {}, "payload": {
            "vins": {"application": { }}
        }})__");
        UNIT_ASSERT(!patch.Apply(event, sessionContext));
    } {
        NAliceProtocol::TSessionContext sessionContext;
        SetAppId(sessionContext, "A");
        auto event = CreateJson(R"__({"header": {}, "payload": {
            "vins": {"application": { }}
        }})__");
        UNIT_ASSERT(patch.Apply(event, sessionContext));
    }
}

}; // Y_UNIT_TEST_SUITE(PatchFunctions_IfPayload)
