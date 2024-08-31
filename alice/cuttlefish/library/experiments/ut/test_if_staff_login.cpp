#include <alice/cuttlefish/library/experiments/experiment_patch.h>
#include <alice/cuttlefish/library/experiments/ut/common.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/json/json_reader.h>
#include <util/folder/path.h>

using namespace NVoice::NExperiments;


Y_UNIT_TEST_SUITE(PatchFunctions_IfStaffLogin) {

Y_UNIT_TEST(IfHasStaffLogin) {
    const TExpPatch patch = CreateExperimentPatch(R"__(["if_has_staff_login"])__");
    NJson::TJsonValue event = CreateJson(R"__({"header": {}, "payload": {}})__");

    UNIT_ASSERT(!patch.Apply(event, {}));  // no staff login
    UNIT_ASSERT(patch.Apply(event, CreateSessionContext({}, "X")));
}

Y_UNIT_TEST(IfStaffLoginEq_InvalidArgument) {
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "if_staff_login_eq" ])__"), yexception);
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "if_staff_login_eq", 12 ])__"), yexception);
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "if_staff_login_eq", [] ])__"), yexception);
}

Y_UNIT_TEST(IfStaffLoginEq) {
    const TExpPatch patch = CreateExperimentPatch(R"__(["if_staff_login_eq", "A"])__");
    NJson::TJsonValue event = CreateJson(R"__({"header": {}, "payload": {}})__");

    UNIT_ASSERT(!patch.Apply(event, {}));  // no staff login
    UNIT_ASSERT(!patch.Apply(event, CreateSessionContext({}, "B")));  // wrong staff login
    UNIT_ASSERT(patch.Apply(event, CreateSessionContext({}, "A")));
}

Y_UNIT_TEST(IfStaffLoginIn_InvalidArgument) {
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "if_staff_login_in" ])__"), yexception);
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "if_staff_login_in", 12 ])__"), yexception);
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "if_staff_login_in", "X" ])__"), yexception);
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "if_staff_login_in", {"X": "Y"} ])__"), yexception);
    UNIT_ASSERT_EXCEPTION(CreateExperimentPatch(R"__([ "if_staff_login_in", ["X", "X"] ])__"), yexception);
}

Y_UNIT_TEST(IfStaffLoginIn) {
    const TExpPatch patch = CreateExperimentPatch(R"__(["if_staff_login_in", ["A", "B", "C"]])__");
    NJson::TJsonValue event = CreateJson(R"__({"header": {}, "payload": {}})__");

    UNIT_ASSERT(!patch.Apply(event, {}));  // no staff login
    UNIT_ASSERT(!patch.Apply(event, CreateSessionContext({}, "D")));  // wrong staff login
    UNIT_ASSERT(patch.Apply(event, CreateSessionContext({}, "A")));
    UNIT_ASSERT(patch.Apply(event, CreateSessionContext({}, "C")));
}

}; // Y_UNIT_TEST_SUITE(PatchFunctions_IfStaffLogin)
