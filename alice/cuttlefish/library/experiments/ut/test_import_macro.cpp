#include <alice/cuttlefish/library/experiments/experiment_patch.h>
#include <alice/cuttlefish/library/experiments/ut/common.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/json/json_reader.h>
#include <util/folder/path.h>

using namespace NVoice::NExperiments;

namespace {

 const TExpContext EXP_CONTEXT = {
        CreateJson(R"__({
            "FirstMacro": [ "one", "two", "three" ],
            "SecondMacro": [ "four", "five", "six" ]
        })__").GetMapSafe()
    };

} // anonymous namespace


Y_UNIT_TEST_SUITE(PatchFunctions_ImportMacro) {

Y_UNIT_TEST(InvalidArguments) {
    // too few arguments
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "import_macro" ])__"), yexception);
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "import_macro", ".request.experiments" ])__"), yexception);
    // last argument must be a string
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "import_macro", ".some.path", 1 ])__"), yexception);
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "import_macro", ".some.path", {"key": "value"} ])__"), yexception);
    // invalid macro key
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "import_macro", ".some.path", "ThirdMacro" ])__"), yexception);
}


Y_UNIT_TEST(ImportUsual) {
    CheckPatch(
        // PATCH
        R"__([ "import_macro", ".My.Field", "FirstMacro" ])__",
        // ORIGINAL EVENT
        R"__({
            "header": { },
            "payload": { }
        })__",
        // PATCHED EVENT
        R"__({
            "header": { },
            "payload": {
                "My": { "Field": [ "one", "two", "three" ] }
            }
        })__", EXP_CONTEXT);

    CheckPatch(
        // PATCH
        R"__([ "import_macro", ".My.Field", "SecondMacro" ])__",
        // ORIGINAL EVENT
        R"__({
            "header": { },
            "payload": {
                "My": { "Field": [ "five" ] }
            }
        })__",
        // PATCHED EVENT
        R"__({
            "header": { },
            "payload": {
                "My": {
                    "Field": [ "five", "four", "six" ]
                }
            }
        })__", EXP_CONTEXT);
}

Y_UNIT_TEST(ImportTricky) {
    CheckPatch(
        // PATCH
        R"__([ "import_macro", ".request.experiments", "FirstMacro" ])__",
        // ORIGINAL EVENT
        R"__({
            "header": { },
            "payload": { }
        })__",
        // PATCHED EVENT
        R"__({
            "header": { },
            "payload": {"request": {"experiments": { "one": "1", "two": "1", "three": "1" }}}
        })__", EXP_CONTEXT);
    CheckPatch(
        // PATCH
        R"__([ "import_macro", ".request.experiments", "SecondMacro" ])__",
        // ORIGINAL EVENT
        R"__({
            "header": { },
            "payload": {
                 "request": {"experiments": { "five": 4 }}
            }
        })__",
        // PATCHED EVENT
        R"__({
            "header": { },
            "payload": {"request": {"experiments": { "four": "1", "five": 4, "six": "1" }}}
        })__", EXP_CONTEXT);
}

}; // Y_UNIT_TEST_SUITE(PatchFunctions_ImportMacro)
