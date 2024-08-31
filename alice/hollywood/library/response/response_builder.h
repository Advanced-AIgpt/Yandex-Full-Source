#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/context/context.h>
#include <alice/hollywood/library/nlg/nlg_data.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>

#include <alice/library/analytics/interfaces/analytics_info_builder.h>
#include <alice/library/analytics/scenario/builder.h>
#include <alice/library/util/rng.h>
#include <alice/library/version/version.h>
#include <alice/megamind/protos/common/directive_channel.pb.h>
#include <alice/megamind/protos/common/effect_options.pb.h>
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/scenarios/action_space.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/frame.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/megamind/protos/scenarios/stack_engine.pb.h>
#include <alice/memento/proto/api.pb.h>
#include <alice/protos/api/renderer/api.pb.h>

#include <google/protobuf/wrappers.pb.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/maybe.h>

#include <memory>

namespace NAlice::NHollywood {

constexpr TStringBuf MULTIROOM_SESSION_ID = "multiroom_session_id";
constexpr TStringBuf LOCATION_ID = "location_id";
constexpr TStringBuf LOCATION_ROOMS_IDS = "location_rooms_ids";
constexpr TStringBuf LOCATION_DEVICES_IDS = "location_devices_ids";
constexpr TStringBuf LOCATION_GROUPS_IDS = "location_groups_ids";
constexpr TStringBuf LOCATION_EVERYWHERE = "location_everywhere";
constexpr TStringBuf LOCATION_SMART_SPEAKER_MODELS = "location_smart_speaker_models";
constexpr TStringBuf LOCATION_INCLUDE_CURRENT_DEVICE_ID = "location_include_current_device_id";

// TODO(a-square): builder for TTheme when we need it
class TActionButtonBuilder {
public:
    using TProto = NScenarios::TLayout::TSuggest::TActionButton;

public:
    TActionButtonBuilder(TProto& proto, const TString& actionId)
        : Proto_(proto)
    {
        Y_ENSURE(actionId, "ActionId cannot be empty");
        Proto_.SetActionId(actionId);
    }

    TActionButtonBuilder& Title(const TString& title) {
        Proto_.SetTitle(title);
        return *this;
    }

    TActionButtonBuilder& ActionId(const TString& actionId) {
        Y_ENSURE(actionId, "ActionId cannot be empty");
        Proto_.SetActionId(actionId);
        return *this;
    }

private:
    TProto& Proto_;
};

class TSearchButtonBuilder {
public:
    using TProto = NScenarios::TLayout::TSuggest::TSearchButton;

public:
    explicit TSearchButtonBuilder(TProto& proto)
        : Proto_(proto)
    {}

    TSearchButtonBuilder& Title(const TString& title) {
        Proto_.SetTitle(title);
        return *this;
    }

    TSearchButtonBuilder& Query(const TString& query) {
        Proto_.SetQuery(query);
        return *this;
    }

private:
    TProto& Proto_;
};

enum class EShouldListen {
    Default = 0,
    ForcedYes,
    ForcedNo,
};

enum EShowViewLayer {
    Content = 0,
    Dialog,
    Alarm,
};

class TResetAddBuilder {
public:
    using TProto = NScenarios::TStackEngineAction::TResetAdd;

public:
    explicit TResetAddBuilder(TProto& proto)
        : Proto_(proto)
    {}

    NScenarios::TParsedUtterance& AddUtterance(const TString& utterance, EShouldListen shouldListen = {}, TDirectiveChannel::EDirectiveChannel channel = {}, TStringBuf forcedEmotion = {}) {
        auto& parsedUtterance = *AddEffect(shouldListen, channel, forcedEmotion).MutableParsedUtterance();
        if (!utterance.empty()) {
            parsedUtterance.SetUtterance(utterance);
        }
        return parsedUtterance;
    }

    NScenarios::TCallbackDirective& AddCallback(const TString& name, TDirectiveChannel::EDirectiveChannel channel = {}, TStringBuf forcedEmotion = {}) {
        auto* callback = AddEffect({}, channel, forcedEmotion).MutableCallback();
        callback->SetName(name);
        return *callback;
    }

    NScenarios::TCallbackDirective& AddRecoveryActionCallback(const TString& name,
                                                              ::google::protobuf::Struct&& payload) {
        auto& callback = *Proto_.MutableRecoveryAction()->MutableCallback();
        callback.SetName(name);
        *callback.MutablePayload() = std::move(payload);
        return callback;
    }

private:
    TProto& Proto_;

    NScenarios::TStackEngineEffect& AddEffect(EShouldListen shouldListen, TDirectiveChannel::EDirectiveChannel channel, TStringBuf forcedEmotion) {
        auto& effect = *Proto_.AddEffects();
        if (shouldListen != EShouldListen::Default) {
            effect.MutableOptions()->MutableForcedShouldListen()->set_value(shouldListen == EShouldListen::ForcedYes);
        }
        if (channel) {
            effect.MutableOptions()->SetChannel(channel);
        }
        if (forcedEmotion) {
            effect.MutableOptions()->SetForcedEmotion(TString{forcedEmotion});
        }
        return effect;
    }
};

class IResponseBodyRenderer {
public:
    virtual ~IResponseBodyRenderer() = default;
    virtual void AddRenderedText(NScenarios::TScenarioResponseBody& responseBody, const TString& text) const = 0;
    virtual void AddRenderedVoice(NScenarios::TScenarioResponseBody& responseBody, const TString& voice) const = 0;
    virtual void AddRenderedTextWithButtons(NScenarios::TScenarioResponseBody& responseBody, const TString& text,
                                            const TVector<NScenarios::TLayout::TButton>& buttons) const = 0;
};

class TCommonResponseBodyRenderer : public IResponseBodyRenderer {
public:
    void AddRenderedText(NScenarios::TScenarioResponseBody& responseBody, const TString& text) const override;
    void AddRenderedVoice(NScenarios::TScenarioResponseBody& responseBody, const TString& voice) const override;
    void AddRenderedTextWithButtons(NScenarios::TScenarioResponseBody& responseBody, const TString& text,
                                    const TVector<NScenarios::TLayout::TButton>& buttons) const override;
};

class TSilentResponseBodyRenderer : public IResponseBodyRenderer {
public:
    void AddRenderedText(NScenarios::TScenarioResponseBody& /* responseBody */, const TString& /* text */) const override {};
    void AddRenderedVoice(NScenarios::TScenarioResponseBody& /* responseBody */, const TString& /* voice */) const override {};
    void AddRenderedTextWithButtons(NScenarios::TScenarioResponseBody& /* responseBody */, const TString& /* text */,
                                    const TVector<NScenarios::TLayout::TButton>& /* buttons */) const override {};
};

// instead of writing Text/Voice for himself, it creates directive to push RepeatAfterMe semantic frame
class TRepeatAfterMeResponseBodyRenderer: public IResponseBodyRenderer {
public:
    struct TRedirectData {
        TString Puid;
        TString DeviceId;
        ui32 Ttl;
    };

public:
    TRepeatAfterMeResponseBodyRenderer(TRedirectData redirectData);

    void AddRenderedText(NScenarios::TScenarioResponseBody& responseBody, const TString& text) const override;
    void AddRenderedVoice(NScenarios::TScenarioResponseBody& responseBody, const TString& voice) const override;
    void AddRenderedTextWithButtons(NScenarios::TScenarioResponseBody& responseBody, const TString& text,
                                    const TVector<NScenarios::TLayout::TButton>& buttons) const override;

private:
    TRedirectData RedirectData_;
};

// TODO(vitvlkv): add unit tests
class TResponseBodyBuilder {
public:
    using TRenderData = THashMap<TString, NRenderer::TDivRenderData>;

public:
    TResponseBodyBuilder(NScenarios::TScenarioResponseBody& response,
                         TNlgWrapper* nlg,
                         const TFrame* frame = nullptr,
                         std::unique_ptr<IResponseBodyRenderer> renderer = std::make_unique<TCommonResponseBodyRenderer>());

    // TODO(a-square): using plain protobufs for data is bad, switch to model classes and/or builders
    struct TSuggest {
        TVector<NScenarios::TDirective> Directives;
        TMaybe<NScenarios::TDirective> AutoDirective;
        TMaybe<TString> ButtonForText;
        TMaybe<TString> SuggestButton;
    };

    NScenarios::IAnalyticsInfoBuilder& CreateAnalyticsInfoBuilder();
    NScenarios::IAnalyticsInfoBuilder& CreateAnalyticsInfoBuilder(const NScenarios::TAnalyticsInfo& analyticsInfo);
    NScenarios::IAnalyticsInfoBuilder& CreateAnalyticsInfoBuilder(NScenarios::TAnalyticsInfo&& analyticsInfo);
    NScenarios::IAnalyticsInfoBuilder& GetAnalyticsInfoBuilder();
    bool HasAnalyticsInfoBuilder() const;
    NScenarios::IAnalyticsInfoBuilder& GetOrCreateAnalyticsInfoBuilder() {
        return HasAnalyticsInfoBuilder() ? GetAnalyticsInfoBuilder() : CreateAnalyticsInfoBuilder();
    }

    NNlg::TRenderPhraseResult RenderPhrase(
        const TStringBuf nlgTemplateName,
        const TStringBuf phraseName,
        const TNlgData& nlgData) const;

    void TryAddRenderedVoice(
        const TStringBuf nlgTemplateName,
        const TStringBuf phraseName,
        const TNlgData& nlgData);

    void AddRenderedVoice(
        const TStringBuf nlgTemplateName,
        const TStringBuf phraseName,
        const TNlgData& nlgData);

    void AddRenderedText(
        const TStringBuf nlgTemplateName,
        const TStringBuf phraseName,
        const TNlgData& nlgData);

    void AddRenderedTextWithButtons(
        const TStringBuf nlgTemplateName,
        const TStringBuf phraseName,
        const TVector<NScenarios::TLayout::TButton>& buttons,
        const TNlgData& nlgData);

    void TryAddRenderedTextWithButtonsAndVoice(
        const TStringBuf nlgTemplateName,
        const TStringBuf phraseName,
        const TVector<NScenarios::TLayout::TButton>& buttons,
        const TNlgData& nlgData);

    void AddRenderedTextWithButtonsAndVoice(
        const TStringBuf nlgTemplateName,
        const TStringBuf phraseName,
        const TVector<NScenarios::TLayout::TButton>& buttons,
        const TNlgData& nlgData);

    void AddRenderedTextAndVoice(const TStringBuf nlgTemplateName, const TStringBuf phraseName, const TNlgData& nlgData) {
        AddRenderedTextWithButtonsAndVoice(nlgTemplateName, phraseName, Default<TVector<NScenarios::TLayout::TButton>>(), nlgData);
    }

    void AddRawTextWithButtonsAndVoice(
        const TString& text,
        const TVector<NScenarios::TLayout::TButton>& buttons);

    void AddRawTextWithButtonsAndVoice(
        const TString& text,
        const TString& voice,
        const TVector<NScenarios::TLayout::TButton>& buttons);

    void AddRenderedDivCard(
        const TStringBuf nlgTemplateName,
        const TStringBuf cardName,
        const TNlgData& nlgData,
        const bool reduceWhitespace = false);

    void AddRenderedDiv2Card(
        const TStringBuf nlgTemplateName,
        const TStringBuf cardName,
        const TNlgData& nlgData,
        const bool reduceWhitespace = false);

    // TODO(a-square): AddAction actually mutates actionId - refactor!
    void AddAction(NScenarios::TDirective&& directive, TString& actionId);
    void AddAction(NScenarios::TDirective&& directive, TFrameNluHint&& nluHint, TString& actionId);
    void AddNluHint(TFrameNluHint&& nluHint);

    void AddClientActionDirective(const TString& name, const TString& analyticsType, const NJson::TJsonValue& value); // TODO(vitvlkv): Avoid TJsonValues here, use Protos
    void AddClientActionDirective(const TString& name, const NJson::TJsonValue& value); // TODO(vitvlkv): Avoid TJsonValues here, use Protos

    void AddDoNotDisturbOnDirective();
    void AddDoNotDisturbOffDirective();

    void AddSetTimerDirectiveForTurnOff(ui64 duration);

    void AddRenderedSuggest(TResponseBodyBuilder::TSuggest&& suggest);
    void AddTypeTextSuggest(const TString& text, const TMaybe<TString>& typeText = Nothing(), const TMaybe<TString>& name = Nothing());

    void AddWidgetRenderData(NData::TScenarioData& scenarioData);

    void AddTtsPlayPlaceholderDirective(const NJson::TJsonValue& value = NJson::TJsonValue());
    void AddOpenUriDirective(const TString& uri, const TStringBuf screen_id="", const TStringBuf name="open_uri");
    void AddShowPromoDirective();
    bool TryAddAuthorizationDirective(const bool supportsOpenYandexAuth);
    bool TryAddTtsPlayPlaceholderDirective();
    void AddListenDirective(TMaybe<int> silenceTimeoutMs = {});
    bool HasListenDirective() const;

    void AddWebViewMediaSessionPlayDirective(const TString& mediaSessionId);
    void AddWebViewMediaSessionPauseDirective(const TString& mediaSessionId);

    void AddAction(const TString& actionId, NScenarios::TFrameAction&& action);
    void AddShowViewDirective(
        NRenderer::TDivRenderData&& renderData,
        NScenarios::TShowViewDirective_EInactivityTimeout inactivityTimeout = NScenarios::TShowViewDirective_EInactivityTimeout_Short,
        EShowViewLayer layer = Dialog);
    void AddCardDirective(NRenderer::TDivRenderData&& teaserRenderData, const TString& actionSpaceId = "",
                          const NScenarios::TAddCardDirective_EChromeLayerType chromeLayerType = NScenarios::TAddCardDirective_EChromeLayerType::TAddCardDirective_EChromeLayerType_Default);
    void AddCardDirectiveWithTeaserTypeAndId(NRenderer::TDivRenderData&& teaserRenderData, const TString& teaserType, const TString& actionSpaceId = "", const TString& teaserId = "",
                          const NScenarios::TAddCardDirective_EChromeLayerType chromeLayerType = NScenarios::TAddCardDirective_EChromeLayerType::TAddCardDirective_EChromeLayerType_Default);
    void AddCardDirective(NRenderer::TDivRenderData&& teaserRenderData,
                          const NScenarios::TAddCardDirective_EChromeLayerType chromeLayerType, const TString& actionSpaceId = "");
    void AddCardDirectiveWithTeaserTypeAndId(NRenderer::TDivRenderData&& teaserRenderData,
                          const NScenarios::TAddCardDirective_EChromeLayerType chromeLayerType, const TString& teaserType, const TString& teaserId = "", const TString& actionSpaceId = "");
    void AddDirective(NScenarios::TDirective&& directive);
    void AddServerDirective(NScenarios::TServerDirective&& directive);
    bool TryAddMementoUserConfig(const ru::yandex::alice::memento::proto::EConfigKey key, const NProtoBuf::Message& value);

    TActionButtonBuilder AddActionSuggest(const TString& actionId);
    TSearchButtonBuilder AddSearchSuggest();
    TResetAddBuilder ResetAddBuilder();
    void AddNewSessionStackAction();

    void SetExpectsRequest(bool expectsRequest);
    void SetShouldListen(bool shouldListen);

    void SetState(const ::google::protobuf::Message& state);

    NScenarios::TScenarioResponseBody& GetResponseBody() { return ResponseBody_; }

    const TRenderData& GetRenderData() const { return RenderData_; }
    TRenderData&& MoveRenderData() { return std::move(RenderData_); }

    void AddActionSpace(const TString& str, const NScenarios::TActionSpace& actionSpace);
    void AddScenarioData(const NData::TScenarioData& scenarioData);
    void AddContextualData(const NData::TContextualData& contextualData);
    void AddRenderData(NRenderer::TDivRenderData&& renderData);

    void SetResponseLanguage(const ELanguage responseLanguage);
    void SetIsResponseConjugated(const bool isConjugated);

    void Build();

private:
    void AddRenderedText(const TString& text);
    void AddRenderedVoice(const TString& voice);
    void AddRenderedTextWithButtons(const TString& text, const TVector<NScenarios::TLayout::TButton>& buttons);

private:
    NScenarios::TScenarioResponseBody& ResponseBody_;
    TNlgWrapper* Nlg_ = nullptr;
    std::unique_ptr<IResponseBodyRenderer> Renderer_;
    TMaybe<NScenarios::TAnalyticsInfoBuilder> AnalyticsInfoBuilder_;
    TVector<NScenarios::TLayout::TButton> ButtonsForText_;
    THashMap<TString, NRenderer::TDivRenderData> RenderData_;
};

struct IResponseBuilder {
    virtual ~IResponseBuilder() = default;

    virtual TNlgWrapper& GetNlgWrapper() = 0;

    virtual TResponseBodyBuilder& GetOrCreateResponseBodyBuilder(const TFrame* frame = nullptr) = 0;
    virtual void SetError(const TString& type, const TString& message) = 0;

    virtual void AddPlayerFeatures(NAlice::NScenarios::TScenarioRunResponse_TFeatures_TPlayerFeatures&& /* playerFeatures */) {}
};

template <typename TResponseProto>
class TCommonResponseBuilder : public IResponseBuilder {
public:
    explicit TCommonResponseBuilder(TNlgWrapper* nlg = nullptr)
        : Response_{std::make_unique<TProto>()}
        , Nlg_{nlg}
    {
        Response_->SetVersion(VERSION_STRING);
    }

    virtual std::unique_ptr<TResponseProto> BuildResponse() && {
        return std::move(Response_);
    }

    TNlgWrapper& GetNlgWrapper() override {
        Y_ENSURE(Nlg_, "You must SetNlgRegistration on a scenario for which you want to render phrases and cards");
        return *Nlg_;
    }

    void SetError(const TString& type, const TString& message) override {
        EnsureProtoUnset();
        auto& error = *Response_->MutableError();
        error.SetType(type);
        error.SetMessage(message);
    }

    TResponseBodyBuilder& GetOrCreateResponseBodyBuilder(const TFrame* frame = nullptr) override = 0;

protected:
    void EnsureProtoUnset() const {
        Y_ENSURE(Response_->GetResponseCase() == TProto::RESPONSE_NOT_SET);
    }

protected:
    using TProto = TResponseProto;

    std::unique_ptr<TResponseProto> Response_;
    TNlgWrapper* Nlg_ = nullptr;
};

template <typename TProto>
class TCommonResponseBuilderWithBody : public TCommonResponseBuilder<TProto> {
public:
    using TBase = TCommonResponseBuilder<TProto>;
public:
    explicit TCommonResponseBuilderWithBody(TNlgWrapper* nlg = nullptr,
                                            std::unique_ptr<IResponseBodyRenderer> renderer = std::make_unique<TCommonResponseBodyRenderer>())
        : TBase(nlg)
        , Renderer_(std::move(renderer))
    {}

    TResponseBodyBuilder* GetResponseBodyBuilder() {
        return BodyBuilder_.get();
    }

    std::unique_ptr<TProto> BuildResponse() && override {
        if (auto* bodyBuilder = GetResponseBodyBuilder()) {
            bodyBuilder->Build();
        }
        return std::move(*this).TBase::BuildResponse();
    }

    TResponseBodyBuilder& CreateResponseBodyBuilder(const TFrame* frame = nullptr) {
        this->EnsureProtoUnset();
        auto& responseBody = *this->Response_->MutableResponseBody();
        this->BodyBuilder_ = std::make_unique<TResponseBodyBuilder>(responseBody, this->Nlg_, frame, std::move(Renderer_));
        return *this->BodyBuilder_;
    }

    TResponseBodyBuilder& GetOrCreateResponseBodyBuilder(const TFrame* frame = nullptr) override {
        if (auto* bodyBuilder = GetResponseBodyBuilder()) {
            return *bodyBuilder;
        }
        return CreateResponseBodyBuilder(frame);
    }

protected:
    std::unique_ptr<TResponseBodyBuilder> BodyBuilder_;
    std::unique_ptr<IResponseBodyRenderer> Renderer_;
};

class TRunResponseBuilder : public TCommonResponseBuilderWithBody<NScenarios::TScenarioRunResponse> {
public:
    using TCommonResponseBuilderWithBody<TProto>::TCommonResponseBuilderWithBody;

    void SetApplyArguments(const google::protobuf::Message& args);
    void SetContinueArguments(const google::protobuf::Message& args);
    TResponseBodyBuilder& CreateCommitCandidate(const google::protobuf::Message& args, const TFrame* frame = nullptr);

    // NOTE: When use SetIrrelevant(), do not forget to put a response for user into the ResponseBody
    void SetIrrelevant();

    static std::unique_ptr<NScenarios::TScenarioRunResponse>
    MakeIrrelevantResponse(TNlgWrapper& nlg,
                           const TStringBuf message,
                           std::unique_ptr<IResponseBodyRenderer> renderer = std::make_unique<TCommonResponseBodyRenderer>());
    void SetFeaturesIntent(const TString& intent);

    NAlice::NScenarios::TScenarioRunResponse_TFeatures& GetMutableFeatures();
    ui32 FillMusicFeatures(const TStringBuf searchText, const NJson::TJsonValue& searchResults, bool isPlayerCommand);
    void FillPlayerFeatures(bool restorePlayer, ui32 secondsSincePause);

    void AddPlayerFeatures(NAlice::NScenarios::TScenarioRunResponse_TFeatures_TPlayerFeatures&& playerFeatures) override {
        *Response_->MutableFeatures()->MutablePlayerFeatures() = std::move(playerFeatures);
    }
};

class THwFrameworkRunResponseBuilder : public TRunResponseBuilder {
private:
    using TBase = TRunResponseBuilder;

public:
    template<typename... TArgs>
    THwFrameworkRunResponseBuilder(const TScenarioHandleContext& ctx, TArgs&&... args)
        : TBase{std::forward<TArgs>(args)...}
        , Ctx_{ctx}
    {}

    // proxy functions to the base with patched args
    void SetContinueArguments(const google::protobuf::Message& args) {
        TBase::SetContinueArguments(PrepareArguments(args, Ctx_.NewContext));
    }

    void SetApplyArguments(const google::protobuf::Message& args) {
        TBase::SetApplyArguments(PrepareArguments(args, Ctx_.NewContext));
    }

    TResponseBodyBuilder& CreateCommitCandidate(const google::protobuf::Message& args, const TFrame* frame = nullptr) {
        return TBase::CreateCommitCandidate(PrepareArguments(args, Ctx_.NewContext), frame);
    }

    // implicit functions (for rare cases)
    void SetContinueArgumentsOldFlow(const google::protobuf::Message& args) {
        TBase::SetContinueArguments(PrepareOldFlowArguments(args));
    }

    void SetApplyArgumentsOldFlow(const google::protobuf::Message& args) {
        TBase::SetApplyArguments(PrepareOldFlowArguments(args));
    }

    TResponseBodyBuilder& CreateCommitCandidateOldFlow(const google::protobuf::Message& args, const TFrame* frame = nullptr) {
        return TBase::CreateCommitCandidate(PrepareOldFlowArguments(args), frame);
    }

private:
    const TScenarioHandleContext& Ctx_;
};

using TApplyResponseBuilder = TCommonResponseBuilderWithBody<NScenarios::TScenarioApplyResponse>;
using TContinueResponseBuilder = TCommonResponseBuilderWithBody<NScenarios::TScenarioContinueResponse>;

class TCommitResponseBuilder : public TCommonResponseBuilder<NScenarios::TScenarioCommitResponse> {
public:
    explicit TCommitResponseBuilder() = default;

    TResponseBodyBuilder& GetOrCreateResponseBodyBuilder(const TFrame* frame = nullptr) override;
    void SetSuccess();
};

template<typename T>
concept TProtoResponseBuilder = requires (T t) {
    std::is_base_of_v<IResponseBuilder, T>;
    std::move(t).BuildResponse();
};

} // namespace NAlice::NHollywood
