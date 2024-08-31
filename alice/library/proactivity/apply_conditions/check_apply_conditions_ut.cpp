#include <check_apply_conditions.h>

#include <alice/library/json/json.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/string/join.h>

using namespace NAlice;

namespace {

// HashMap to store expected output as (postrollHasComponent, responseHasComponent) -> expectedValue
using TConditionChecker = THashMap<bool, THashMap<bool, TMaybe<bool>>>;

const TConditionChecker CONDITION_MATCH_RESULT= {
    {/* postrollHasComponent */ true, {{/* responseHasComponent */ true, true}, {/* responseHasComponent */ false, false}}},
    {/* postrollHasComponent */ false, {{/* responseHasComponent */ true, true}, {/* responseHasComponent */ false, true}}}
};

const TConditionChecker CONDITION_IS_PRESENT = {
    {/* postrollHasComponent */ true, {{/* responseHasComponent */ true, true}, {/* responseHasComponent */ false, false}}},
    {/* postrollHasComponent */ false, {{/* responseHasComponent */ true, true}, {/* responseHasComponent */ false, false}}}
};

const TConditionChecker CONDITION_IS_ABSENT = {
    {/* postrollHasComponent */ true, {{/* responseHasComponent */ true, false}, {/* responseHasComponent */ false, true}}},
    {/* postrollHasComponent */ false, {{/* responseHasComponent */ true, false}, {/* responseHasComponent */ false, true}}}
};

const TConditionChecker CONDITION_IGNORE = {
    {/* postrollHasComponent */ true, {{/* responseHasComponent */ true, true}, {/* responseHasComponent */ false, true}}},
    {/* postrollHasComponent */ false, {{/* responseHasComponent */ true, true}, {/* responseHasComponent */ false, true}}}
};

const TConditionChecker CONDITION_UNKOWN = {
    {/* postrollHasComponent */ true, {{/* responseHasComponent */ true, Nothing()}, {/* responseHasComponent */ false, Nothing()}}},
    {/* postrollHasComponent */ false, {{/* responseHasComponent */ true, Nothing()}, {/* responseHasComponent */ false, Nothing()}}}
};

const TVector<TString> DIRECTIVE_NAMES_PRESENT_IN_RESPONSE = {"directive_name1", "directive_name2"};

constexpr auto DIRECTIVE_COND_NAME_MATCHED_AND_IS_PRESENT = TStringBuf(R"(
    {
        "Name": "directive_name1",
        "Status": "IsPresent"
    }
)");

constexpr auto DIRECTIVE_COND_NAME_MATCHED_AND_IS_ABSENT = TStringBuf(R"(
    {
        "Name": "directive_name2",
        "Status": "IsAbsent"
    }
)");

constexpr auto DIRECTIVE_COND_NAME_NOT_MATCHED_AND_IS_PRESENT = TStringBuf(R"(
    {
        "Name": "not_directive_name1",
        "Status": "IsPresent"
    }
)");

constexpr auto DIRECTIVE_COND_NAME_NOT_MATCHED_AND_IS_ABSENT = TStringBuf(R"(
    {
        "Name": "not_directive_name2",
        "Status": "IsAbsent"
    }
)");

void TestCheckApplyConditionResponseComponentIsOK(const NDJ::NAS::TApplyCondition::EResponseVoiceOrText condition,
                                                  const TConditionChecker& expectedCondition) {
    for (int postrollHasComponent = 0; postrollHasComponent < 2; ++postrollHasComponent) {
        for (int responseHasComponent = 0; responseHasComponent < 2; ++responseHasComponent) {
            const TMaybe<bool> actualOutput = CheckApplyConditionResponseComponentIsOk(condition, postrollHasComponent, responseHasComponent);

            UNIT_ASSERT_C(
                expectedCondition.contains(postrollHasComponent) && expectedCondition.at(postrollHasComponent).contains(responseHasComponent),
                TStringBuilder{} << "Unexpected combination of postrollHasComponent=" << postrollHasComponent << " and responsehasComponent=" << responseHasComponent
                                 << " for condition type " << NDJ::NAS::TApplyCondition::EResponseVoiceOrText_Name(condition));

            UNIT_ASSERT_C(
                actualOutput == expectedCondition.at(postrollHasComponent).at(responseHasComponent),
                TStringBuilder{} << "Apply condition with type " 
                                 << NDJ::NAS::TApplyCondition::EResponseVoiceOrText_Name(condition) 
                                 << " expected to be " << expectedCondition.at(postrollHasComponent).at(responseHasComponent)
                                 << " with postrollHasComponent=" << postrollHasComponent
                                 << " and responseHasComponent=" << responseHasComponent);
        }
    }
}

void TestCheckApplyConditionResponseDirectiveIsOk(const TStringBuf directiveConditionString,
                                                  const TVector<TString>& responseDirectiveNames,
                                                  const bool expectedOutput) {
    NDJ::NAS::TApplyCondition::TDirectiveCondition condition;
    google::protobuf::RepeatedPtrField<NAlice::NSpeechKit::TDirective> directives;

    const auto status = JsonToProto(NJson::ReadJsonFastTree(directiveConditionString), condition);
    UNIT_ASSERT_C(status.ok(), status.ToString());

    for (const auto& directiveName : responseDirectiveNames) {
        directives.Add()->SetName(directiveName);
    }

    const bool actualOutput = CheckApplyConditionResponseDirectiveIsOk(condition, directives);

    UNIT_ASSERT_C(
        actualOutput == expectedOutput,
        TStringBuilder{} << "Apply condition for directive " 
                         << JsonStringFromProto(condition) 
                         << " expected to be " 
                         << (expectedOutput ? "true. " : "false. ")
                         << "Current directives are: "
                         << JoinSeq(",", directives));
}

Y_UNIT_TEST_SUITE(CheckApplyConditions) {
    Y_UNIT_TEST(CheckApplyConditionResponseComponentIsOK) {
        TestCheckApplyConditionResponseComponentIsOK(NDJ::NAS::TApplyCondition::MatchResult, CONDITION_MATCH_RESULT);
        TestCheckApplyConditionResponseComponentIsOK(NDJ::NAS::TApplyCondition::IsPresent, CONDITION_IS_PRESENT);
        TestCheckApplyConditionResponseComponentIsOK(NDJ::NAS::TApplyCondition::IsAbsent, CONDITION_IS_ABSENT);
        TestCheckApplyConditionResponseComponentIsOK(NDJ::NAS::TApplyCondition::Ignore, CONDITION_IGNORE);
        TestCheckApplyConditionResponseComponentIsOK(static_cast<NDJ::NAS::TApplyCondition::EResponseVoiceOrText>(1000), CONDITION_UNKOWN);
    }

    Y_UNIT_TEST(CheckApplyConditionDirectiveComponent) {
        TestCheckApplyConditionResponseDirectiveIsOk(DIRECTIVE_COND_NAME_MATCHED_AND_IS_PRESENT, DIRECTIVE_NAMES_PRESENT_IN_RESPONSE, /* expectedOutput */ true);
        TestCheckApplyConditionResponseDirectiveIsOk(DIRECTIVE_COND_NAME_MATCHED_AND_IS_ABSENT, DIRECTIVE_NAMES_PRESENT_IN_RESPONSE, /* expectedOutput */ false);
        TestCheckApplyConditionResponseDirectiveIsOk(DIRECTIVE_COND_NAME_NOT_MATCHED_AND_IS_PRESENT, DIRECTIVE_NAMES_PRESENT_IN_RESPONSE, /* expectedOutput */ false);
        TestCheckApplyConditionResponseDirectiveIsOk(DIRECTIVE_COND_NAME_NOT_MATCHED_AND_IS_ABSENT, DIRECTIVE_NAMES_PRESENT_IN_RESPONSE, /* expectedOutput */ true);
    }
}
} // namespace
