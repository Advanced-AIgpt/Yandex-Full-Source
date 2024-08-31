#pragma once

#include <alice/megamind/library/modifiers/modifier.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NMegamind {

using TModifierCreator = TModifierPtr(void);

class TModifierTestFixture : public NUnitTest::TBaseFixture {
public:
    struct TActionInfo {
        TStringBuf Name;
        TStringBuf FrameName;
    };

    struct TInput {
        TStringBuf Scenario;
        TStringBuf Intent;
        TStringBuf Storage;
        TActionInfo Action;
        TStringBuf PersonalIntents;
        TStringBuf SkrJson;
        TStringBuf BlackBoxJson;
        TVector<TStringBuf> SkillRecs;
        TVector<TStringBuf> SemanticFrames;
        TStringBuf PersonalData;
        bool ShouldListen;
        bool IntentFromSession;
        bool AddStackEngineGetNext;
        bool AddTextReponse = true;
        bool AddVoiceReponse = true;
        TVector<TStringBuf> FramesJsons;
        TStringBuf UserConfigs;
    };

    class TInputBuilder {
    public:
        TInputBuilder& SetScenario(const TStringBuf scenario) {
            Input.Scenario = scenario;
            return *this;
        }
        TInputBuilder& SetIntent(const TStringBuf intent) {
            Input.Intent = intent;
            return *this;
        }
        TInputBuilder& SetStorage(const TStringBuf storage) {
            Input.Storage = storage;
            return *this;
        }
        TInputBuilder& SetAction(const TActionInfo action) {
            Input.Action = action;
            return *this;
        }
        TInputBuilder& SetPersonalIntents(const TStringBuf personalIntents) {
            Input.PersonalIntents = personalIntents;
            return *this;
        }
        TInputBuilder& SetSkrJson(const TStringBuf skrJson) {
            Input.SkrJson = skrJson;
            return *this;
        }
        TInputBuilder& SetBlackBoxJson(const TStringBuf blackBoxJson) {
            Input.BlackBoxJson = blackBoxJson;
            return *this;
        }
        TInputBuilder& SetSkillRec(const TStringBuf skillRec) {
            Input.SkillRecs = {skillRec};
            return *this;
        }
        TInputBuilder& SetSkillRecs(const std::initializer_list<TStringBuf>& skillRecs) {
            Input.SkillRecs = skillRecs;
            return *this;
        }
        TInputBuilder& SetAddStackEngineGetNext(const bool addStackEngineGetNext) {
            Input.AddStackEngineGetNext = addStackEngineGetNext;
            return *this;
        }
        TInputBuilder& SetAddTextReponse(const bool addTextReponse) {
            Input.AddTextReponse = addTextReponse;
            return *this;
        }
        TInputBuilder& SetAddVoiceReponse(const bool addVoiceReponse) {
            Input.AddVoiceReponse = addVoiceReponse;
            return *this;
        }
        TInputBuilder& SetShouldListen(const bool shouldListen) {
            Input.ShouldListen = shouldListen;
            return *this;
        }
        TInputBuilder& SetIntentFromSession(const bool intentFromSession) {
            Input.IntentFromSession = intentFromSession;
            return *this;
        }
        TInputBuilder& SetFramesJsons(const TVector<TStringBuf>& framesJsons) {
            Input.FramesJsons = framesJsons;
            return *this;
        }
        TInputBuilder& SetSemanticFrames(const TVector<TStringBuf>& semanticFrames) {
            Input.SemanticFrames = semanticFrames;
            return *this;
        }
        TInputBuilder& SetPersonalData(const TStringBuf personalData) {
            Input.PersonalData = personalData;
            return *this;
        }
        TInputBuilder& SetUserConfigs(const TStringBuf userConfigs) {
            Input.UserConfigs = userConfigs;
            return *this;
        }

        TInput Build() {
            return std::move(Input);
        }

    private:
        TInput Input;
    };

    void TestExpectedNonApply(
        TModifierCreator creator,
        const TInput& input,
        const TNonApply::EType expectedType,
        const TStringBuf expectedReason = {}
    );

    void TestExpectedAction(
        TModifierCreator creator,
        const TInput& input,
        const TStringBuf expectedName,
        const TStringBuf expectedAction
    );

    void TestExpectedResponse(
        TModifierCreator creator,
        const TInput& input,
        const TStringBuf expected
    );

    void TestExpectedResponseNonApply(
        TModifierCreator creator,
        const TInput& input,
        const TStringBuf expected,
        const TNonApply::EType expectedNonApplyType,
        const TStringBuf expectedNonApplyReason
    );

    void TestExpectedTts(
        TModifierCreator creator,
        const TInput& input,
        const TStringBuf expected
    );

    void TestExpectedIncRateCall(
        TModifierCreator creator,
        const TInput& input,
        const TString expectedItemInfo,
        const TString expectedSource
    );

    void TestExpectedTextAndVoice(
        TModifierCreator creator,
        const TInput& input,
        const TStringBuf expectedText,
        const TStringBuf expectedVoice
    );

    void TestExpectedLogStorage(
        TModifierCreator creator,
        const TInput& input,
        const TStringBuf expected
    );

    void TestExpectedLogStorageNonApply(
        TModifierCreator creator,
        const TInput& input,
        const TStringBuf expectedLog,
        const TNonApply::EType expectedNonApplyType,
        const TStringBuf expectedNonApplyReason
    );

    void TestPostrollAnalytics(
        TModifierCreator creator,
        const TInput& input,
        const TStringBuf expectedAnalyticsJson
    );

    void TestExpectedProactivityInfo(
        TModifierCreator creator,
        const TInput& input,
        const TStringBuf expectedProactivityInfo
    );
};

} // NAlice::NMegamind
