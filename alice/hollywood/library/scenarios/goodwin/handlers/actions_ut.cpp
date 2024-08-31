#include "actions.h"

#include <alice/library/unittest/message_diff.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/protos/data/language/language.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice {
namespace NFrameFiller {
namespace NGoodwin {

namespace {

NScenarios::TDirective MakeCallbackDirective(const TString& name) {
    NScenarios::TDirective directive;
    directive.MutableCallbackDirective()->SetName(name);
    return directive;
}

NScenarios::TDirective MakeOpenUriDirective(const TString& name) {
    NScenarios::TDirective directive;
    directive.MutableOpenUriDirective()->SetName(name);
    return directive;
}

::google::protobuf::RepeatedPtrField<TNluPhrase> MakeInstances(const TVector<TString>& phrases) {
    ::google::protobuf::RepeatedPtrField<TNluPhrase> instances;
    for (const auto& phrase : phrases) {
        auto& instance = *instances.Add();
        instance.SetLanguage(ELang::L_RUS);
        instance.SetPhrase(phrase);
    }
    return instances;
}

TAction MakeAction(
    const ::google::protobuf::RepeatedPtrField<TNluPhrase>& instances,
    const TVector<NScenarios::TDirective> directives
) {
    TAction action;

    *action.MutableNluHint()->MutableInstances() = instances;

    for (const auto& directive : directives) {
        *action.AddDirectives() = directive;
    }

    return action;
}

#define UNIT_ASSERT_MESSAGE_SEQUENCES_EQUAL(lhs, rhs)           \
    do {                                                        \
        UNIT_ASSERT_VALUES_EQUAL((lhs).size(), (rhs).size());   \
        for (int i = 0; i < (lhs).size(); ++i) {                \
            UNIT_ASSERT_MESSAGES_EQUAL((lhs)[i], (rhs)[i]);     \
        }                                                       \
    } while (false)

Y_UNIT_TEST_SUITE(ToFrameAction) {

Y_UNIT_TEST(NoDirectives) {
    const auto instances = MakeInstances({"hello", "bye"});
    const TAction action = MakeAction(instances, {});
    const TString actionName = "doit!";
    const NScenarios::TFrameAction frameAction = ToFrameAction(action, actionName);
    UNIT_ASSERT_VALUES_EQUAL(frameAction.GetNluHint().GetFrameName(), actionName);
    UNIT_ASSERT_MESSAGE_SEQUENCES_EQUAL(frameAction.GetNluHint().GetInstances(), instances);
    UNIT_ASSERT(!frameAction.HasDirectives());
    UNIT_ASSERT(!frameAction.HasCallback());
    UNIT_ASSERT(!frameAction.HasFrame());
}

Y_UNIT_TEST(SingleCallbackDirective) {
    const auto directive = MakeCallbackDirective("callback");
    const auto instances = MakeInstances({"hello", "bye"});
    const TAction action = MakeAction(instances, {directive});
    const TString actionName = "doit!";
    const NScenarios::TFrameAction frameAction = ToFrameAction(action, actionName);
    UNIT_ASSERT_VALUES_EQUAL(frameAction.GetNluHint().GetFrameName(), actionName);
    UNIT_ASSERT_MESSAGE_SEQUENCES_EQUAL(frameAction.GetNluHint().GetInstances(), instances);
    UNIT_ASSERT(!frameAction.HasDirectives());
    UNIT_ASSERT(frameAction.HasCallback());
    UNIT_ASSERT(!frameAction.HasFrame());
    UNIT_ASSERT_MESSAGES_EQUAL(frameAction.GetCallback(), directive.GetCallbackDirective());
}

Y_UNIT_TEST(SingleNonCallbackDirective) {
    const auto directive = MakeOpenUriDirective("open_uri");
    const auto instances = MakeInstances({"hello", "bye"});
    const TAction action = MakeAction(instances, {directive});
    const TString actionName = "open!";
    const NScenarios::TFrameAction frameAction = ToFrameAction(action, actionName);
    UNIT_ASSERT_VALUES_EQUAL(frameAction.GetNluHint().GetFrameName(), actionName);
    UNIT_ASSERT_MESSAGE_SEQUENCES_EQUAL(frameAction.GetNluHint().GetInstances(), instances);
    UNIT_ASSERT_VALUES_EQUAL(frameAction.GetDirectives().GetList().size(), 1);
    UNIT_ASSERT(!frameAction.HasCallback());
    UNIT_ASSERT(!frameAction.HasFrame());
    UNIT_ASSERT_MESSAGES_EQUAL(frameAction.GetDirectives().GetList(0), directive);
}

Y_UNIT_TEST(TwoCallbackDirectives) {
    const auto directive = MakeCallbackDirective("open_uri");
    const auto instances = MakeInstances({"hello", "bye"});
    const TAction action = MakeAction(instances, {directive, directive});
    const TString actionName = "doit!doit!";
    const NScenarios::TFrameAction frameAction = ToFrameAction(action, actionName);
    UNIT_ASSERT_VALUES_EQUAL(frameAction.GetNluHint().GetFrameName(), actionName);
    UNIT_ASSERT_MESSAGE_SEQUENCES_EQUAL(frameAction.GetNluHint().GetInstances(), instances);
    UNIT_ASSERT_VALUES_EQUAL(frameAction.GetDirectives().GetList().size(), 2);
    UNIT_ASSERT(!frameAction.HasCallback());
    UNIT_ASSERT(!frameAction.HasFrame());
    UNIT_ASSERT_MESSAGES_EQUAL(frameAction.GetDirectives().GetList(0), directive);
    UNIT_ASSERT_MESSAGES_EQUAL(frameAction.GetDirectives().GetList(1), directive);
}

}

} // namespace

} // namespace NGoodwin
} // namespace NFrameFiller
} // namespace NAlice
