#pragma once

#include <alice/megamind/library/scenarios/interface/response_builder.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice {

class TMockResponseBuilder : public IResponseBuilder {
public:
    MOCK_METHOD(IResponseBuilder&, AddSimpleTextImpl, (const TString&, bool), ());
    virtual IResponseBuilder& AddSimpleText(const TString& text, bool appendTts = false) override {
        return AddSimpleTextImpl(text, appendTts);
    }
    MOCK_METHOD(IResponseBuilder&, AddSimpleTextImpl, (const TString&, const TString&, bool), ());
    virtual IResponseBuilder& AddSimpleText(const TString& text, const TString& tts, bool appendTts = false) override {
        return AddSimpleTextImpl(text, tts, appendTts);
    }
    MOCK_METHOD(IResponseBuilder&, ShouldListen, (bool), (override));
    MOCK_METHOD(IResponseBuilder&, SetIsTrashPartial, (bool), (override));
    MOCK_METHOD(IResponseBuilder&, SetContentProperties, (const TContentProperties&), (override));
    MOCK_METHOD(IResponseBuilder&, AddError, (const TString&, const TString&), (override));
    MOCK_METHOD(IResponseBuilder&, AddAnalyticsAttention, (const TString&), (override));
    MOCK_METHOD(IResponseBuilder&, AddMeta, (const TString&, const NJson::TJsonValue&), (override));
    MOCK_METHOD(IResponseBuilder&, AddDirective, (const NMegamind::IDirectiveModel&), (override));
    MOCK_METHOD(IResponseBuilder&, AddDirectiveToVoiceResponse, (const NMegamind::IDirectiveModel&), (override));
    MOCK_METHOD(IResponseBuilder&, AddCard, (const NMegamind::ICardModel&), (override));
    MOCK_METHOD(IResponseBuilder&, AddSuggest, (const NMegamind::IButtonModel&), (override));
    MOCK_METHOD(IResponseBuilder&, SetOutputSpeech, (const TString&), (override));
    MOCK_METHOD(IResponseBuilder&, SetDirectivesExecutionPolicy, (EDirectivesExecutionPolicy value), (override));
    MOCK_METHOD(IResponseBuilder&, SetResponseErrorMessage, (const TResponseErrorMessage&), (override));
};

} // namespace NAlice
