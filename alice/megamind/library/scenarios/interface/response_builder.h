#pragma once

#include <alice/megamind/library/models/directives/defer_apply_directive_model.h>
#include <alice/megamind/library/models/directives/update_datasync_directive_model.h>
#include <alice/megamind/library/models/interfaces/button_model.h>
#include <alice/megamind/library/models/interfaces/card_model.h>
#include <alice/megamind/library/models/interfaces/directive_model.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <library/cpp/json/json_value.h>

#include <util/datetime/base.h>
#include <util/generic/fwd.h>
#include <util/generic/noncopyable.h>

namespace NAlice {

class TRenderResult;

class IResponseBuilder {
public:
    virtual ~IResponseBuilder() = default;

    virtual IResponseBuilder& AddSimpleText(const TString& text, bool appendTts = false) = 0;
    virtual IResponseBuilder& AddSimpleText(const TString& text, const TString& tts, bool appendTts = false) = 0;

    virtual IResponseBuilder& AddCard(const NMegamind::ICardModel& card) = 0;
    virtual IResponseBuilder& AddDirective(const NMegamind::IDirectiveModel& directive) = 0;
    virtual IResponseBuilder& AddFrontDirective(const NMegamind::IDirectiveModel& directive) = 0;
    virtual IResponseBuilder& AddDirectiveToVoiceResponse(const NMegamind::IDirectiveModel& directive) = 0;
    virtual IResponseBuilder& AddSuggest(const NMegamind::IButtonModel& button) = 0;
    virtual IResponseBuilder& SetOutputSpeech(const TString& text) = 0;
    virtual IResponseBuilder& SetOutputEmotion(const TString& outputEmotion) = 0;
    virtual IResponseBuilder& SetResponseErrorMessage(const TResponseErrorMessage& responseErrorMessage) = 0;

    virtual IResponseBuilder& ShouldListen(bool value) = 0;
    virtual IResponseBuilder& SetIsTrashPartial(bool value) = 0;
    virtual IResponseBuilder& SetContentProperties(const TContentProperties& contentProperties) = 0;

    virtual IResponseBuilder& AddError(const TString& errorType, const TString& message) = 0;
    virtual IResponseBuilder& AddAnalyticsAttention(const TString& type) = 0;

    virtual IResponseBuilder& AddMeta(const TString& type, const NJson::TJsonValue& payload) = 0;

    virtual IResponseBuilder& SetDirectivesExecutionPolicy(EDirectivesExecutionPolicy value) = 0;
    virtual IResponseBuilder& SetProductScenarioName(const TString& psn) = 0;

    virtual IResponseBuilder& AddProtobufUniproxyDirective(const NMegamind::IDirectiveModel& model) = 0;
};

} // namespace NAlice
