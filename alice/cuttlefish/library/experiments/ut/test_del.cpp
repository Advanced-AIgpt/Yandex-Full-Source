#include <alice/cuttlefish/library/experiments/experiment_patch.h>
#include <alice/cuttlefish/library/experiments/ut/common.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/json/json_reader.h>
#include <util/folder/path.h>

using namespace NVoice::NExperiments;


Y_UNIT_TEST_SUITE(PatchFunctions_Del) {

Y_UNIT_TEST(InvalidArguments) {
    // too few arguments
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__(["del"])__"), yexception);
    // invalid argument types
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__(["del", ["not a string"]])__"), yexception);
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__(["del", 2])__"), yexception);
}

Y_UNIT_TEST(DelNonExisting) {
    CheckPatch(
        R"__(["del", ".AbsentField"])__",
        R"__({"header": { }, "payload": {"ExistingField": 4}})__",
        R"__({"header": { }, "payload": {"ExistingField": 4}})__"
    );
}

Y_UNIT_TEST(DelMap) {
    CheckPatch(
        R"__(["del", ".MapField"])__",
        R"__({
            "header": { },
            "payload": {
                "MapField": { "Field": 1 }
            }
        })__",
        R"__({
            "header": { },
            "payload": { }
        })__"
    );
}


Y_UNIT_TEST(DelDeep) {
    CheckPatch(
        R"__(["del", ".F1.F2.F3.X"])__",
        R"__({
            "header": { },
            "payload": {
                "F1": {
                    "F2": {
                        "F3": {
                            "X": []
                        }
                    }
                }
            }
        })__",
        R"__({
            "header": { },
            "payload": {
                "F1": {
                    "F2": {
                        "F3": { }
                    }
                }
            }
        })__"
    );
}

}; // Y_UNIT_TEST_SUITE(PatchFunctions_Del)
