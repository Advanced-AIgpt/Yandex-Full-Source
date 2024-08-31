#include "common.h"

#include <alice/cuttlefish/library/experiments/flags_json.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NVoice::NExperiments;

Y_UNIT_TEST_SUITE(TFlagsJsonFlagsTests) {
    Y_UNIT_TEST(FullRsp) {
        const TString rsp = (
            R"__({
                    "all": {
                        "CONTEXT": {
                            "MAIN": {
                                "VOICE": {
                                    "flags": {
                                        "key1": 0.0,
                                        "key2": 2,
                                        "key3": "value3"
                                    }
                                },
                                "ASR": {
                                    "flags": ["asr_flag_1", "asr_flag_2"]
                                },
                                "BIO": {
                                    "flags": 1
                                }
                            }
                        }
                    },
                    "exp_config_version": "100500",
                    "reqid": "ponchik",
                    "exp_boxes": "1,0,1;2,0,-2"
            })__"
        );

        NAliceProtocol::TSessionContext ctx;
        NAliceProtocol::TFlagsInfo* info = ctx.MutableExperiments()->MutableFlagsJsonData()->MutableFlagsInfo();
        UNIT_ASSERT(ParseFlagsInfoFromRawResponse(info, rsp));

        TFlagsJsonFlagsPtr flags = MakeFlagsConstRefFromSessionContextProto(ctx);

        UNIT_ASSERT(!flags->ConductingExperiment("key1"));
        UNIT_ASSERT_VALUES_EQUAL(flags->GetExperimentValueInteger("key1"), Nothing());
        UNIT_ASSERT_VALUES_EQUAL(flags->GetExperimentValueFloat("key1"), 0.0);

        UNIT_ASSERT(flags->ConductingExperiment("key2"));
        UNIT_ASSERT_VALUES_EQUAL(flags->GetExperimentValueBoolean("key2"), Nothing());
        UNIT_ASSERT_VALUES_EQUAL(flags->GetExperimentValueFloat("key2"), Nothing());
        UNIT_ASSERT_VALUES_EQUAL(flags->GetExperimentValueString("key2"), Nothing());
        UNIT_ASSERT_VALUES_EQUAL(flags->GetExperimentValueInteger("key2"), 2);

        UNIT_ASSERT(flags->ConductingExperiment("key3"));
        UNIT_ASSERT_VALUES_EQUAL(flags->GetExperimentValueString("key3"), "value3");
        UNIT_ASSERT_VALUES_EQUAL(flags->GetExperimentValueBoolean("key3"), Nothing());

        UNIT_ASSERT_VALUES_EQUAL(
            flags->GetAsrAbFlagsSerializedJson(),
            R"({"ASR":{"boxes":"1,0,1;2,0,-2","flags":["asr_flag_1","asr_flag_2"]}})"
        );
        UNIT_ASSERT_VALUES_EQUAL(flags->GetBioAbFlagsSerializedJson(), Nothing());

        UNIT_ASSERT_VALUES_EQUAL(flags->GetExpConfigVersion(), 100500);
        UNIT_ASSERT_VALUES_EQUAL(flags->GetRequestId(), "ponchik");
        UNIT_ASSERT_VALUES_EQUAL(flags->GetExpBoxes().GetRef(), "1,0,1;2,0,-2");

        NJson::TJsonValue::TMapType voiceFlagsMap = flags->GetVoiceFlagsJson().GetMap();
        UNIT_ASSERT_VALUES_EQUAL(voiceFlagsMap.size(), 3);
        UNIT_ASSERT_VALUES_EQUAL(voiceFlagsMap["key1"], 0.0);
        UNIT_ASSERT_VALUES_EQUAL(voiceFlagsMap["key2"], 2);
        UNIT_ASSERT_VALUES_EQUAL(voiceFlagsMap["key3"], "value3");
    }

    Y_UNIT_TEST(BrokenRsp) {
        const TString rsp = (
            R"__({
                    "all": {
                        "CONTEXT": {
                            "MAIN": {
                                "VOICE": {
                                    "flags": ["key1", "key2"]
                                }
                            }
                        }
                    }
            })__"
        );

        NAliceProtocol::TSessionContext ctx;
        NAliceProtocol::TFlagsInfo* info = ctx.MutableExperiments()->MutableFlagsJsonData()->MutableFlagsInfo();
        UNIT_ASSERT(!ParseFlagsInfoFromRawResponse(info, rsp));

        TFlagsJsonFlagsPtr flags = MakeFlagsConstRefFromSessionContextProto(ctx);

        UNIT_ASSERT(flags->ConductingExperiment("key1"));
        UNIT_ASSERT(flags->ConductingExperiment("key2"));
        UNIT_ASSERT(!flags->ConductingExperiment("key3"));
        UNIT_ASSERT_VALUES_EQUAL(flags->GetExperimentValueString("key1"), "1");
        UNIT_ASSERT_VALUES_EQUAL(flags->GetExperimentValueBoolean("key3"), Nothing());
        UNIT_ASSERT_VALUES_EQUAL(flags->GetExperimentValueFloat("key3"), Nothing());
        UNIT_ASSERT_VALUES_EQUAL(flags->GetExperimentValueInteger("key3"), Nothing());
        UNIT_ASSERT_VALUES_EQUAL(flags->GetExperimentValueString("key3"), Nothing());
    }
};
