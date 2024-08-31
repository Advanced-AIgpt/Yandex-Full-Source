#include "response_builder.h"

#include "alice/protos/data/scenario/centaur/teasers/teaser_settings.pb.h"
#include "music_features.h"

#include <alice/hollywood/library/global_context/global_context.h>

#include <alice/library/analytics/scenario/builder.h>
#include <alice/library/json/json.h>
#include <alice/library/response/defs.h>
#include <alice/library/url_builder/url_builder.h>
#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/megamind/protos/scenarios/action_space.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/protos/data/contextual_data.pb.h>
#include <alice/protos/data/device/info.pb.h>
#include <alice/protos/data/language/language.pb.h>
#include <alice/protos/data/scenario/centaur/main_screen.pb.h>
#include <alice/protos/data/scenario/data.pb.h>
#include <alice/protos/analytics/dummy_response/response.pb.h>

#include <util/string/join.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

TRepeatAfterMeSemanticFrame& GetOrCreateRepeatAfterMeSemanticFrame(NScenarios::TScenarioResponseBody& responseBody,
                                                                   const TRepeatAfterMeResponseBodyRenderer::TRedirectData &redirectData) {
    for (auto& serverDirective : *responseBody.MutableServerDirectives()) {
        if (serverDirective.HasPushTypedSemanticFrameDirective()) {
            return *serverDirective.MutablePushTypedSemanticFrameDirective()->
                MutableSemanticFrameRequestData()->
                MutableTypedSemanticFrame()->
                MutableRepeatAfterMeSemanticFrame();
        }
    }

    auto& directive = *responseBody.AddServerDirectives()->MutablePushTypedSemanticFrameDirective();
    directive.SetPuid(redirectData.Puid);
    directive.SetDeviceId(redirectData.DeviceId);
    directive.SetTtl(redirectData.Ttl);

    auto& requestData = *directive.MutableSemanticFrameRequestData();

    auto& analytics = *requestData.MutableAnalytics();
    analytics.SetOrigin(TAnalyticsTrackingModule::Scenario);
    analytics.SetPurpose("multiroom_mock_redirect");

    return *requestData.MutableTypedSemanticFrame()->MutableRepeatAfterMeSemanticFrame();
}

void SetOrAppendString(TStringSlot& slot, const TString& value) {
    if (slot.HasStringValue()) {
        const auto& previousValue = slot.GetStringValue();
        slot.SetStringValue(TString::Join(previousValue, "\n", value));
    } else {
        slot.SetStringValue(value);
    }
}


template <class TTypedDirective>
void SetMultiroomSessionIdIfPresent(TTypedDirective& typedDirective, const NJson::TJsonValue& value) {
    if (const auto* multiroomSessionId = value.GetValueByPath(MULTIROOM_SESSION_ID)) {
        typedDirective.SetMultiroomSessionId(multiroomSessionId->GetString());
    }
}


using TProtoArrayOfStrings = ::google::protobuf::RepeatedPtrField<TProtoStringType>;
void CopyArrayOfStrings(const NJson::TJsonValue::TArray& from, TProtoArrayOfStrings* to) {
    for (const auto& el : from) {
        *to->Add() = el.GetString();
    }
}


template <class TTypedDirective>
void FillLocationInfo(TTypedDirective& directive, const NJson::TJsonValue& value,
                      const TStringBuf fieldNameForDeprecatedRoomId = LOCATION_ID)
{
    if (const auto* roomId = value.GetValueByPath(fieldNameForDeprecatedRoomId)) {
        directive.SetRoomId(roomId->GetString());
    }

    if (const auto* roomsIds = value.GetValueByPath(LOCATION_ROOMS_IDS)) {
        for (const auto& el : roomsIds->GetArray()) {
            if (el.GetString() == "everywhere") {
                directive.MutableLocationInfo()->SetEverywhere(true);
            } else {
                directive.MutableLocationInfo()->AddRoomsIds(el.GetString());
            }
        }
    }
    if (const auto* groupsIds = value.GetValueByPath(LOCATION_GROUPS_IDS)) {
        CopyArrayOfStrings(groupsIds->GetArray(), directive.MutableLocationInfo()->MutableGroupsIds());
    }
    if (const auto* devicesIds = value.GetValueByPath(LOCATION_DEVICES_IDS)) {
        CopyArrayOfStrings(devicesIds->GetArray(), directive.MutableLocationInfo()->MutableDevicesIds());
    }
    if (const auto* modelsTypes = value.GetValueByPath(LOCATION_SMART_SPEAKER_MODELS)) {
        for (const auto& el : modelsTypes->GetArray()) {
            const auto smartSpeakerModel = static_cast<EUserDeviceType>(el.GetInteger());
            directive.MutableLocationInfo()->AddSmartSpeakerModels(smartSpeakerModel);
        }
    }
    if (value.GetValueByPath(LOCATION_EVERYWHERE)) {
        directive.MutableLocationInfo()->SetEverywhere(true);
    }
    if (value.GetValueByPath(LOCATION_INCLUDE_CURRENT_DEVICE_ID)) {
        directive.MutableLocationInfo()->SetIncludeCurrentDeviceId(true);
    }
}


void PackFrom(google::protobuf::Any& dest, const google::protobuf::Message& src) {
    if (const auto* anySrc = google::protobuf::DynamicCastToGenerated<google::protobuf::Any>(&src)) {
        dest = *anySrc;
    } else {
        dest.PackFrom(src);
    }
}


} // anonymous namespace

// TCommonResponseBodyRenderer ---------------------------------------------------------
void TCommonResponseBodyRenderer::AddRenderedText(NScenarios::TScenarioResponseBody& responseBody, const TString& text) const {
    auto& layout = *responseBody.MutableLayout();
    layout.AddCards()->SetText(text);
}

void TCommonResponseBodyRenderer::AddRenderedVoice(NScenarios::TScenarioResponseBody& responseBody, const TString& voice) const {
    auto& layout = *responseBody.MutableLayout();

    if (const auto& previousValue = layout.GetOutputSpeech()) {
        layout.SetOutputSpeech(TStringBuilder() << previousValue << '\n' << voice);
    } else {
        layout.SetOutputSpeech(voice);
    }
}

void TCommonResponseBodyRenderer::AddRenderedTextWithButtons(NScenarios::TScenarioResponseBody& responseBody, const TString& text,
                                                             const TVector<NScenarios::TLayout::TButton>& buttons) const {
    if (buttons.empty()) {
        AddRenderedText(responseBody, text);
        return;
    }

    auto& layout = *responseBody.MutableLayout();
    auto& textWithButtons = *layout.AddCards()->MutableTextWithButtons();
    textWithButtons.SetText(text);

    for (const auto& button : buttons) {
        auto* buttonProto = textWithButtons.AddButtons();
        buttonProto->CopyFrom(button);
    }
}

// TRepeatAfterMeResponseBodyRenderer ---------------------------------------------------------
TRepeatAfterMeResponseBodyRenderer::TRepeatAfterMeResponseBodyRenderer(TRedirectData redirectData)
    : RedirectData_(std::move(redirectData))
{
}

void TRepeatAfterMeResponseBodyRenderer::AddRenderedText(NScenarios::TScenarioResponseBody& responseBody, const TString& text) const {
    auto& repeatAfterMeFrame = GetOrCreateRepeatAfterMeSemanticFrame(responseBody, RedirectData_);
    SetOrAppendString(*repeatAfterMeFrame.MutableText(), text);
}

void TRepeatAfterMeResponseBodyRenderer::AddRenderedVoice(NScenarios::TScenarioResponseBody& responseBody, const TString& voice) const {
    auto& repeatAfterMeFrame = GetOrCreateRepeatAfterMeSemanticFrame(responseBody, RedirectData_);
    SetOrAppendString(*repeatAfterMeFrame.MutableVoice(), voice);
}

void TRepeatAfterMeResponseBodyRenderer::AddRenderedTextWithButtons(NScenarios::TScenarioResponseBody& responseBody, const TString& text,
                                                                    const TVector<NScenarios::TLayout::TButton>& buttons) const {
    Y_UNUSED(buttons); // mocks use only text and voice
    AddRenderedText(responseBody, text);
}

// TRunResponseBuilder ---------------------------------------------------------
void TRunResponseBuilder::SetApplyArguments(const google::protobuf::Message& args) {
    this->EnsureProtoUnset();
    PackFrom(*Response_->MutableApplyArguments(), args);
}

void TRunResponseBuilder::SetContinueArguments(const google::protobuf::Message& args) {
    this->EnsureProtoUnset();
    PackFrom(*Response_->MutableContinueArguments(), args);
}

TResponseBodyBuilder& TRunResponseBuilder::CreateCommitCandidate(const google::protobuf::Message& args,
                                                                 const TFrame* frame) {
    this->EnsureProtoUnset();
    auto& commitCandidate = *Response_->MutableCommitCandidate();
    PackFrom(*commitCandidate.MutableArguments(), args);

    auto& responseBody = *commitCandidate.MutableResponseBody();
    BodyBuilder_ = std::make_unique<TResponseBodyBuilder>(responseBody, Nlg_, frame, std::move(Renderer_));
    return *BodyBuilder_;
}

void TRunResponseBuilder::SetIrrelevant() {
    Response_->MutableFeatures()->SetIsIrrelevant(true);
}

std::unique_ptr<NScenarios::TScenarioRunResponse> TRunResponseBuilder::MakeIrrelevantResponse(TNlgWrapper& nlg,
    const TStringBuf message,
    std::unique_ptr<IResponseBodyRenderer> renderer) {
    TRunResponseBuilder response(&nlg, std::move(renderer));
    response.SetIrrelevant();
    auto& bodyBuilder = response.CreateResponseBodyBuilder();
    bodyBuilder.AddRawTextWithButtonsAndVoice(TString(message), /* buttons = */ {});
    return std::move(response).BuildResponse();
}

void TRunResponseBuilder::SetFeaturesIntent(const TString& intent) {
    Response_->MutableFeatures()->SetIntent(intent);
}

NAlice::NScenarios::TScenarioRunResponse_TFeatures& TRunResponseBuilder::GetMutableFeatures() {
    return *Response_->MutableFeatures();
}

ui32 TRunResponseBuilder::FillMusicFeatures(const TStringBuf searchText, const NJson::TJsonValue& searchResults, bool isPlayerCommand) {
    // filling features multiple times is definitely an error
    Y_ENSURE(Response_->GetFeatures().GetFeaturesCase() == TScenarioRunResponse::TFeatures::FEATURES_NOT_SET);
    return FillMusicFeaturesProto(
        searchText,
        searchResults,
        isPlayerCommand,
        *Response_->MutableFeatures()->MutableMusicFeatures()
    );
}

void TRunResponseBuilder::FillPlayerFeatures(bool restorePlayer, ui32 secondsSincePause) {
    auto& mutableFeatures = *Response_->MutableFeatures()->MutablePlayerFeatures();
    mutableFeatures.SetRestorePlayer(restorePlayer);
    mutableFeatures.SetSecondsSincePause(secondsSincePause);
}

// TCommitResponseBuilder ------------------------------------------------------

TResponseBodyBuilder& TCommitResponseBuilder::GetOrCreateResponseBodyBuilder(const TFrame*) {
    ythrow yexception() << "TCommitResponseBuilder doesn't have a body";
}

void TCommitResponseBuilder::SetSuccess() {
    EnsureProtoUnset();
    (void)Response_->MutableSuccess(); // creates the oneof case
}

// TResponseBodyBuilder --------------------------------------------------------
TResponseBodyBuilder::TResponseBodyBuilder(TScenarioResponseBody& responseBody,
                                           TNlgWrapper* nlg,
                                           const TFrame* frame,
                                           std::unique_ptr<IResponseBodyRenderer> renderer)
    : ResponseBody_(responseBody)
    , Nlg_(nlg)
    , Renderer_(std::move(renderer))
{
    if (frame != nullptr) {
        // WARNING(a-square): the frame may come from a variety of places, we just assume that Begemot won't choke on it,
        // it's the responsibility of the scenario developers to actually verify that.
        *ResponseBody_.MutableSemanticFrame() = frame->ToProto();
    }
}

IAnalyticsInfoBuilder& TResponseBodyBuilder::CreateAnalyticsInfoBuilder() {
    Y_ENSURE(!AnalyticsInfoBuilder_);
    Y_ENSURE(!ResponseBody_.HasAnalyticsInfo());

    auto& bodyAnalyticsInfo = *ResponseBody_.MutableAnalyticsInfo();
    AnalyticsInfoBuilder_.ConstructInPlace(bodyAnalyticsInfo);
    return *AnalyticsInfoBuilder_;
}

IAnalyticsInfoBuilder& TResponseBodyBuilder::CreateAnalyticsInfoBuilder(const TAnalyticsInfo& analyticsInfo) {
    Y_ENSURE(!AnalyticsInfoBuilder_);
    Y_ENSURE(!ResponseBody_.HasAnalyticsInfo());

    auto& bodyAnalyticsInfo = *ResponseBody_.MutableAnalyticsInfo();
    bodyAnalyticsInfo = analyticsInfo;
    AnalyticsInfoBuilder_.ConstructInPlace(bodyAnalyticsInfo);
    return *AnalyticsInfoBuilder_;
}

IAnalyticsInfoBuilder& TResponseBodyBuilder::CreateAnalyticsInfoBuilder(TAnalyticsInfo&& analyticsInfo) {
    Y_ENSURE(!AnalyticsInfoBuilder_);
    Y_ENSURE(!ResponseBody_.HasAnalyticsInfo());

    auto& bodyAnalyticsInfo = *ResponseBody_.MutableAnalyticsInfo();
    bodyAnalyticsInfo = std::move(analyticsInfo);
    AnalyticsInfoBuilder_.ConstructInPlace(bodyAnalyticsInfo);
    return *AnalyticsInfoBuilder_;
}

IAnalyticsInfoBuilder& TResponseBodyBuilder::GetAnalyticsInfoBuilder() {
    Y_ENSURE(AnalyticsInfoBuilder_);

    return *AnalyticsInfoBuilder_;
}

bool TResponseBodyBuilder::HasAnalyticsInfoBuilder() const {
    return AnalyticsInfoBuilder_.Defined();
}

NNlg::TRenderPhraseResult TResponseBodyBuilder::RenderPhrase(const TStringBuf nlgTemplateName,
                                                       const TStringBuf phraseName,
                                                       const TNlgData& nlgData) const
{
    Y_ENSURE(Nlg_, "Nlg wrapper is null");
    return Nlg_->RenderPhrase(nlgTemplateName, phraseName, nlgData);
}

void TResponseBodyBuilder::TryAddRenderedVoice(const TStringBuf nlgTemplateName,
                                               const TStringBuf phraseName,
                                               const TNlgData& nlgData)
{
    if (Nlg_ && Nlg_->HasPhrase(nlgTemplateName, phraseName)) {
        AddRenderedVoice(nlgTemplateName, phraseName, nlgData);
    }
}

void TResponseBodyBuilder::AddRenderedVoice(const TStringBuf nlgTemplateName,
                                            const TStringBuf phraseName,
                                            const TNlgData& nlgData)
{
    AddRenderedVoice(RenderPhrase(nlgTemplateName, phraseName, nlgData).Voice);
}

void TResponseBodyBuilder::AddRenderedText(const TStringBuf nlgTemplateName,
                                           const TStringBuf phraseName,
                                           const TNlgData& nlgData)
{
    AddRenderedText(RenderPhrase(nlgTemplateName, phraseName, nlgData).Text);
}

void TResponseBodyBuilder::AddRenderedTextWithButtons(const TStringBuf nlgTemplateName,
                                                      const TStringBuf phraseName,
                                                      const TVector<TLayout::TButton>& buttons,
                                                      const TNlgData& nlgData)
{
    AddRenderedTextWithButtons(RenderPhrase(nlgTemplateName, phraseName, nlgData).Text, buttons);
}

void TResponseBodyBuilder::TryAddRenderedTextWithButtonsAndVoice(const TStringBuf nlgTemplateName,
                                                                 const TStringBuf phraseName,
                                                                 const TVector<TLayout::TButton>& buttons,
                                                                 const TNlgData& nlgData)
{
    if (Nlg_ && Nlg_->HasPhrase(nlgTemplateName, phraseName)) {
        AddRenderedTextWithButtonsAndVoice(nlgTemplateName, phraseName, buttons, nlgData);
    }
}

void TResponseBodyBuilder::AddRenderedTextWithButtonsAndVoice(const TStringBuf nlgTemplateName,
                                                              const TStringBuf phraseName,
                                                              const TVector<TLayout::TButton>& buttons,
                                                              const TNlgData& nlgData)
{
    auto result = RenderPhrase(nlgTemplateName, phraseName, nlgData);
    if (!result.Text.empty() || !buttons.empty()) {
        AddRenderedTextWithButtons(result.Text, buttons);
    }
    if (!result.Voice.empty()) {
        AddRenderedVoice(result.Voice);
    }
}

void TResponseBodyBuilder::AddRawTextWithButtonsAndVoice(const TString& text,
                                                         const TVector<NScenarios::TLayout::TButton>& buttons) {
    AddRawTextWithButtonsAndVoice(text, text, buttons);
}

void TResponseBodyBuilder::AddRawTextWithButtonsAndVoice(const TString& text, const TString& voice,
                                                         const TVector<NScenarios::TLayout::TButton>& buttons) {
    AddRenderedTextWithButtons(text, buttons);
    AddRenderedVoice(voice);
}

void TResponseBodyBuilder::AddRenderedDivCard(const TStringBuf nlgTemplateName, const TStringBuf cardName,
                                              const TNlgData& nlgData, const bool reduceWhitespace)
{
    Y_ENSURE(Nlg_, "Nlg wrapper is null");
    const auto result = Nlg_->RenderCard(nlgTemplateName, cardName, nlgData, reduceWhitespace);

    auto& layout = *ResponseBody_.MutableLayout();
    google::protobuf::Struct message;
    if (JsonToProto(result.Card, message).ok()) {
        layout.AddCards()->MutableDivCard()->CopyFrom(message);
    }
}

void TResponseBodyBuilder::AddRenderedDiv2Card(const TStringBuf nlgTemplateName, const TStringBuf cardName,
                                              const TNlgData& nlgData, const bool reduceWhitespace)
{
    Y_ENSURE(Nlg_, "Nlg wrapper is null");
    const auto result = Nlg_->RenderCard(nlgTemplateName, cardName, nlgData, reduceWhitespace);
    auto& layout = *ResponseBody_.MutableLayout();
    google::protobuf::Struct cardBody;
    google::protobuf::Struct cardTemplate;
    if (JsonToProto(result.Card["card"], cardBody).ok() && JsonToProto(result.Card["templates"], cardTemplate).ok()) {
        auto& card = *layout.AddCards()->MutableDiv2CardExtended();
        card.MutableBody()->CopyFrom(cardBody);
        card.SetHideBorders(result.Card["hide_borders"].GetBoolean());
        layout.MutableDiv2Templates()->MergeFrom(cardTemplate);
    }
}

void TResponseBodyBuilder::AddAction(TDirective&& directive, TString& actionId) {
    auto& actions = *ResponseBody_.MutableFrameActions();
    if (!actionId) {
        int lastAction = actions.size();
        actionId = ToString(lastAction + 1);
    }

    if (directive.HasCallbackDirective()) {
        *actions[actionId].MutableCallback() = directive.GetCallbackDirective();
    } else {
        *actions[actionId].MutableDirectives()->AddList() = std::move(directive);
    }
    actions[actionId].MutableNluHint()->SetFrameName(actionId);
}

void TResponseBodyBuilder::AddAction(TDirective&& directive, TFrameNluHint&& nluHint, TString& actionId) {
    AddAction(std::move(directive), actionId);
    *(*ResponseBody_.MutableFrameActions())[actionId].MutableNluHint() = std::move(nluHint);
}

void TResponseBodyBuilder::AddAction(const TString& actionId, TFrameAction&& action) {
    auto& actions = *ResponseBody_.MutableFrameActions();
    Y_ENSURE(!actions.count(actionId));

    actions[actionId] = std::move(action);
}

void TResponseBodyBuilder::AddShowViewDirective(NRenderer::TDivRenderData&& renderData,
                                                TShowViewDirective_EInactivityTimeout inactivityTimeout,
                                                EShowViewLayer layer) {
    TDirective directive;
    auto& showViewDirective = *directive.MutableShowViewDirective();

    showViewDirective.SetName("show_view");
    showViewDirective.SetCardId(renderData.GetCardId());
    showViewDirective.SetInactivityTimeout(inactivityTimeout);
    switch (layer) {
        case Content:
            showViewDirective.MutableLayer()->MutableContent();
            break;
        case Dialog:
            showViewDirective.MutableLayer()->MutableDialog();
            break;
        case Alarm:
            showViewDirective.MutableLayer()->MutableAlarm();
            break;
    }
    showViewDirective.SetDoNotShowCloseButton(true);

    RenderData_[renderData.GetCardId()] = std::move(renderData);
    AddDirective(std::move(directive));
}

void TResponseBodyBuilder::AddCardDirective(NRenderer::TDivRenderData&& teaserRenderData,
                                            const TString& actionSpaceId,
                                            const NScenarios::TAddCardDirective_EChromeLayerType chromeLayerType) {
    TDirective directive;
    auto& addCardDirective = *directive.MutableAddCardDirective();

    addCardDirective.SetName("add_card");
    addCardDirective.SetCardId(teaserRenderData.GetCardId());
    addCardDirective.SetCardShowTimeSec(30u);
    addCardDirective.SetActionSpaceId(actionSpaceId);
    addCardDirective.SetChromeLayerType(chromeLayerType);

    RenderData_[teaserRenderData.GetCardId()] = std::move(teaserRenderData);
    AddDirective(std::move(directive));
}

void TResponseBodyBuilder::AddCardDirectiveWithTeaserTypeAndId(
    NRenderer::TDivRenderData&& teaserRenderData, 
    const TString& teaserType, 
    const TString& actionSpaceId,
    const TString& teaserId,   
    const NScenarios::TAddCardDirective_EChromeLayerType chromeLayerType
) {
    TDirective directive;
    auto& addCardDirective = *directive.MutableAddCardDirective();

    addCardDirective.SetName("add_card");
    addCardDirective.SetCardId(teaserRenderData.GetCardId());
    addCardDirective.SetCardShowTimeSec(30u);
    addCardDirective.SetActionSpaceId(actionSpaceId);
    addCardDirective.SetChromeLayerType(chromeLayerType);

    auto& teaserConfig = *addCardDirective.MutableTeaserConfig();
    teaserConfig.SetTeaserType(teaserType);
    teaserConfig.SetTeaserId(teaserId);

    RenderData_[teaserRenderData.GetCardId()] = std::move(teaserRenderData);
    AddDirective(std::move(directive));
}
    

void TResponseBodyBuilder::AddCardDirective(NRenderer::TDivRenderData&& teaserRenderData,
                                            const NScenarios::TAddCardDirective_EChromeLayerType chromeLayerType,
                                            const TString& actionSpaceId) {
    AddCardDirective(std::move(teaserRenderData), actionSpaceId, chromeLayerType);
}

void TResponseBodyBuilder::AddCardDirectiveWithTeaserTypeAndId(
    NRenderer::TDivRenderData&& teaserRenderData,
    const NScenarios::TAddCardDirective_EChromeLayerType chromeLayerType, 
    const TString& teaserType, 
    const TString& teaserId, 
    const TString& actionSpaceId
) {
    AddCardDirectiveWithTeaserTypeAndId(std::move(teaserRenderData), teaserType, teaserId, actionSpaceId, chromeLayerType);
}
    


void TResponseBodyBuilder::AddActionSpace(const TString& str, const NScenarios::TActionSpace& actionSpace) {
    (*ResponseBody_.MutableActionSpaces())[str] = actionSpace;
}

void TResponseBodyBuilder::AddScenarioData(const NData::TScenarioData& scenarioData) {
    (*ResponseBody_.MutableScenarioData()) = scenarioData;
}

void TResponseBodyBuilder::AddContextualData(const NData::TContextualData& contextualData) {
    ResponseBody_.MutableContextualData()->MergeFrom(contextualData);
}

void TResponseBodyBuilder::SetResponseLanguage(const ELanguage responseLanguage) {
    ResponseBody_.MutableContextualData()->SetResponseLanguage(static_cast<::NAlice::ELang>(responseLanguage));
}

void TResponseBodyBuilder::SetIsResponseConjugated(const bool isConjugated) {
    ResponseBody_.MutableContextualData()->MutableConjugator()->SetResponseConjugationStatus(isConjugated
        ? ::NAlice::NData::TContextualData_TConjugator_EResponseConjugationStatus_Conjugated
        : ::NAlice::NData::TContextualData_TConjugator_EResponseConjugationStatus_Unconjugated
    );
}

void TResponseBodyBuilder::AddRenderData(NRenderer::TDivRenderData&& renderData) {
    RenderData_[renderData.GetCardId()] = std::move(renderData);
}

void TResponseBodyBuilder::AddNluHint(TFrameNluHint&& nluHint) {
    auto& actions = *ResponseBody_.MutableFrameActions();

    const int lastAction = actions.size();
    const TString actionId = ToString(lastAction + 1);

    *actions[actionId].MutableNluHint() = std::move(nluHint);
}

void TResponseBodyBuilder::AddClientActionDirective(const TString& name, const TString& analyticsType,
                                                    const NJson::TJsonValue& value) {
    // TODO(vitvlkv): think about some directive factory or builder?..
    if (name == "music_play") {
        TDirective directive;
        auto& musicPlayDirective = *directive.MutableMusicPlayDirective();
        musicPlayDirective.SetName(analyticsType);
        musicPlayDirective.SetUid(value["uid"].GetStringSafe(""));
        musicPlayDirective.SetSessionId(value["session_id"].GetStringSafe("")); // TODO(vitvlkv): is it mandatory? should be
        musicPlayDirective.SetOffset(value["offset"].GetDoubleSafe(0));
        musicPlayDirective.SetAlarmId(value["alarm_id"].GetStringSafe(""));
        musicPlayDirective.SetFirstTrackId(value["first_track_id"].GetStringSafe(""));
        FillLocationInfo(musicPlayDirective, value, "room_id");
        AddDirective(std::move(directive));
    } else if (name == "open_uri") {
        AddOpenUriDirective(value["uri"].GetString(), "", analyticsType);
    } else if (name == "player_pause") {
        TDirective directive;
        auto& playerPauseDirective = *directive.MutablePlayerPauseDirective();
        playerPauseDirective.SetName(analyticsType);
        playerPauseDirective.SetSmooth(value["smooth"].GetBoolean());
        SetMultiroomSessionIdIfPresent(playerPauseDirective, value);
        FillLocationInfo(playerPauseDirective, value);
        AddDirective(std::move(directive));
    } else if (name == "audio_stop") {
        TDirective directive;
        auto& audioStopDirective = *directive.MutableAudioStopDirective();
        audioStopDirective.SetName(analyticsType);
        audioStopDirective.SetSmooth(value["smooth"].GetBoolean());
        AddDirective(std::move(directive));
    } else if (name == "alarm_stop") {
        TDirective directive;
        auto& alarmStopDirective = *directive.MutableAlarmStopDirective();
        alarmStopDirective.SetName(analyticsType);
        AddDirective(std::move(directive));
    } else if (name == "timer_stop_playing") {
        TDirective directive;
        auto& timerStopPlayingDirective = *directive.MutableTimerStopPlayingDirective();
        timerStopPlayingDirective.SetName(analyticsType);
        timerStopPlayingDirective.SetTimerId(value["timer_id"].GetString());
        AddDirective(std::move(directive));
    } else if (name == "sound_louder") {
        TDirective directive;
        auto& soundLouderDirective = *directive.MutableSoundLouderDirective();
        soundLouderDirective.SetName(analyticsType);
        SetMultiroomSessionIdIfPresent(soundLouderDirective, value);
        FillLocationInfo(soundLouderDirective, value);
        AddDirective(std::move(directive));
    } else if (name == "sound_quiter") {
        TDirective directive;
        auto& soundQuiterDirective = *directive.MutableSoundQuiterDirective();
        soundQuiterDirective.SetName(analyticsType);
        SetMultiroomSessionIdIfPresent(soundQuiterDirective, value);
        FillLocationInfo(soundQuiterDirective, value);
        AddDirective(std::move(directive));
    } else if (name == "sound_mute") {
        TDirective directive;
        auto& soundMuteDirective = *directive.MutableSoundMuteDirective();
        soundMuteDirective.SetName(analyticsType);
        AddDirective(std::move(directive));
    } else if (name == "sound_unmute") {
        TDirective directive;
        auto& soundUnmuteDirective = *directive.MutableSoundUnmuteDirective();
        soundUnmuteDirective.SetName(analyticsType);
        AddDirective(std::move(directive));
    } else if (name == "sound_set_level") {
        TDirective directive;
        auto& soundSetLevelDirective = *directive.MutableSoundSetLevelDirective();
        soundSetLevelDirective.SetName(analyticsType);
        soundSetLevelDirective.SetNewLevel(value["new_level"].GetInteger());
        SetMultiroomSessionIdIfPresent(soundSetLevelDirective, value);
        FillLocationInfo(soundSetLevelDirective, value);
        AddDirective(std::move(directive));
    } else if (name == "draw_led_screen") {
        TDirective directive;
        auto& drawLedScreenDirective = *directive.MutableDrawLedScreenDirective();
        drawLedScreenDirective.SetName(analyticsType);
        for (const auto& image: value["payload"].GetArray()){
            auto& drawItem = *drawLedScreenDirective.AddDrawItem();
            drawItem.SetFrontalLedImage(image["frontal_led_image"].GetString());
            drawItem.SetEndless(image["endless"].GetBoolean());
        }
        AddDirective(std::move(directive));
    } else if (name == "mordovia_show") {
        TDirective directive;
        auto &mordoviaShowDirective = *directive.MutableMordoviaShowDirective();
        mordoviaShowDirective.SetName(analyticsType);
        mordoviaShowDirective.SetUrl(value["url"].GetString());
        mordoviaShowDirective.SetSplashDiv(value["splash_div"].GetString());
        mordoviaShowDirective.SetViewKey(value["view_key"].GetString());
        mordoviaShowDirective.SetGoBack(value["go_back"].GetBoolean());
        auto &callback = *mordoviaShowDirective.MutableCallbackPrototype();
        callback.SetName(value["callback_name"].GetString());
        AddDirective(std::move(directive));
    } else if (name == "mordovia_command") {
        TDirective directive;
        auto& mordoviaCommandDirective = *directive.MutableMordoviaCommandDirective();
        mordoviaCommandDirective.SetName(analyticsType);
        mordoviaCommandDirective.SetCommand(value["command"].GetString());
        mordoviaCommandDirective.SetViewKey(value["view_key"].GetString());
        auto mutableFields = mordoviaCommandDirective.MutableMeta()->mutable_fields();
        for (const auto& field : value["meta"].GetMap()) {
            if (field.second.IsString()) {
                (*mutableFields)[field.first].set_string_value(field.second.GetString());
            } else if (field.second.IsBoolean()) {
                (*mutableFields)[field.first].set_bool_value(field.second.GetBoolean());
            }
        }
        AddDirective(std::move(directive));
    } else if (name == "start_multiroom") {
        TDirective directive;
        auto& startMultiroom = *directive.MutableStartMultiroomDirective();
        startMultiroom.SetName(analyticsType);
        FillLocationInfo(startMultiroom, value, "room_id");
        AddDirective(std::move(directive));
    } else if (name == "car") {
        TDirective directive;
        auto& carDirective = *directive.MutableCarDirective();
        carDirective.SetName(analyticsType);
        carDirective.SetApplication(value["application"].GetString());
        carDirective.SetIntent(value["intent"].GetString());
        if (value.Has("params")) {
            *carDirective.MutableParams() = JsonToProto<google::protobuf::Struct>(value["params"]);
        }
        AddDirective(std::move(directive));
    } else if (name == "player_next_track") {
        TDirective directive;
        auto& playerNextTrackDirective = *directive.MutablePlayerNextTrackDirective();
        playerNextTrackDirective.SetName(analyticsType);
        playerNextTrackDirective.SetUid(value["uid"].GetString());
        playerNextTrackDirective.SetPlayer(value["player"].GetString());
        SetMultiroomSessionIdIfPresent(playerNextTrackDirective, value);
        AddDirective(std::move(directive));
    } else if (name == "player_previous_track") {
        TDirective directive;
        auto& playerPreviousTrackDirective = *directive.MutablePlayerPreviousTrackDirective();
        playerPreviousTrackDirective.SetName(analyticsType);
        playerPreviousTrackDirective.SetPlayer(value["player"].GetString());
        SetMultiroomSessionIdIfPresent(playerPreviousTrackDirective, value);
        AddDirective(std::move(directive));
    } else if (name == "start_image_recognizer"){
        NScenarios::TDirective directive;
        NScenarios::TStartImageRecognizerDirective& startImageRecognizerDirective = *directive.MutableStartImageRecognizerDirective();
        if (value.Has("capture_mode")) {
            startImageRecognizerDirective.SetImageSearchModeName(value["capture_mode"].GetString());
        }
        if (value.Has("camera_type")) {
            startImageRecognizerDirective.SetCameraType(value["camera_type"].GetString());
        }
        AddDirective(std::move(directive));
    } else if (name == "player_shuffle") {
        TDirective directive;
        auto& playerShuffleDirective = *directive.MutablePlayerShuffleDirective();
        playerShuffleDirective.SetName(analyticsType);
        AddDirective(std::move(directive));
    } else if (name == "yandexnavi") {
        TDirective directive;
        auto& yandexNaviDirective = *directive.MutableYandexNaviDirective();
        yandexNaviDirective.SetName(analyticsType);
        yandexNaviDirective.SetApplication(value["application"].GetString());
        yandexNaviDirective.SetIntent(value["intent"].GetString());
        *yandexNaviDirective.MutableParams() = JsonToProto<google::protobuf::Struct>(value["params"]);
        AddDirective(std::move(directive));
    } else if (name == "tts_play_placeholder") {
        TDirective directive;
        auto& ttsPlayPlaceholderDirective = *directive.MutableTtsPlayPlaceholderDirective();
        ttsPlayPlaceholderDirective.SetName(analyticsType);
        AddDirective(std::move(directive));
    } else if (name == "setup_rcu") {
        TDirective directive;
        auto& setupRcuDirective = *directive.MutableSetupRcuDirective();
        setupRcuDirective.SetName(analyticsType);
        AddDirective(std::move(directive));
    } else if (name == "setup_rcu_auto") {
        TDirective directive;
        auto& setupRcuAutoDirective = *directive.MutableSetupRcuAutoDirective();
        setupRcuAutoDirective.SetName(analyticsType);
        if (value.Has("tv_model")) {
            setupRcuAutoDirective.SetTvModel(value["tv_model"].GetString());
        }
        AddDirective(std::move(directive));
    } else if (name == "setup_rcu_check") {
        TDirective directive;
        auto& setupRcuCheckDirective = *directive.MutableSetupRcuCheckDirective();
        setupRcuCheckDirective.SetName(analyticsType);
        AddDirective(std::move(directive));
    } else if (name == "setup_rcu_manual") {
        TDirective directive;
        auto& setupRcuManualDirective= *directive.MutableSetupRcuManualDirective();
        setupRcuManualDirective.SetName(analyticsType);
        AddDirective(std::move(directive));
    } else if (name == "setup_rcu_advanced") {
        TDirective directive;
        auto& setupRcuAdvancedDirective = *directive.MutableSetupRcuAdvancedDirective();
        setupRcuAdvancedDirective.SetName(analyticsType);
        AddDirective(std::move(directive));
    } else if (name == "go_home") {
        TDirective directive;
        auto& goHomeDirective = *directive.MutableGoHomeDirective();
        goHomeDirective.SetName(analyticsType);
        AddDirective(std::move(directive));
    } else if (name == "force_display_cards") {
        TDirective directive;
        auto& forceDisplayCardsDirective = *directive.MutableForceDisplayCardsDirective();
        forceDisplayCardsDirective.SetName(analyticsType);
        AddDirective(std::move(directive));
    } else if (name == "success_starting_onboarding") {
        TDirective directive;
        auto& successStartingOnboardingDirective = *directive.MutableSuccessStartingOnboardingDirective();
        successStartingOnboardingDirective.SetName(analyticsType);
        AddDirective(std::move(directive));
    } else if (name == "clear_queue") {
        TDirective directive;
        auto& clearQueueDirective = *directive.MutableClearQueueDirective();
        clearQueueDirective.SetName(analyticsType);
        AddDirective(std::move(directive));
    } else if (name == "player_continue") {
        TDirective directive;
        auto& playerContinueDirective = *directive.MutablePlayerContinueDirective();
        playerContinueDirective.SetName(analyticsType);
        playerContinueDirective.SetPlayer(value["player"].GetString());
        AddDirective(std::move(directive));
    } else if (name == "power_off") {
        TDirective directive;
        auto& powerOffDirective = *directive.MutablePowerOffDirective();
        powerOffDirective.SetName(analyticsType);
        AddDirective(std::move(directive));
    } else if (name == "screen_off") {
        TDirective directive;
        auto& listenDirective = *directive.MutableScreenOffDirective();
        listenDirective.SetName(analyticsType);
        AddDirective(std::move(directive));
    } else if (name == "save_voiceprint") {
        TDirective directive;
        auto& saveVPDirective = *directive.MutableSaveVoiceprintDirective();
        saveVPDirective.SetUserId(value["user_id"].GetString());
        auto mutableRequests = saveVPDirective.MutableRequestIds();
        for (const auto& request : value["requests"].GetArray()) {
            *mutableRequests->Add() = request.GetString();
        }
        AddDirective(std::move(directive));
    } else if (name == "remove_voiceprint") {
        TDirective directive;
        auto& removeVPDirective = *directive.MutableSaveVoiceprintDirective();
        removeVPDirective.SetUserId(value["user_id"].GetString());
        AddDirective(std::move(directive));
    } else { // it's BASS compatibility function, do not add other directives here
        ythrow yexception() << "Unsupported directive " << name.Quote();
    }
}

void TResponseBodyBuilder::AddClientActionDirective(const TString& name, const NJson::TJsonValue& value) {
    AddClientActionDirective(name, /*analyticsType*/ name, value);
}

void TResponseBodyBuilder::AddDoNotDisturbOnDirective() {
    TDirective directive;
    auto& doNotDisturbOnDirective = *directive.MutableDoNotDisturbOnDirective();
    doNotDisturbOnDirective.SetName("do_not_disturb_on");
    AddDirective(std::move(directive));
}

void TResponseBodyBuilder::AddDoNotDisturbOffDirective() {
    TDirective directive;
    auto& doNotDisturbOffDirective = *directive.MutableDoNotDisturbOffDirective();
    doNotDisturbOffDirective.SetName("do_not_disturb_off");
    AddDirective(std::move(directive));
}

void TResponseBodyBuilder::AddSetTimerDirectiveForTurnOff(ui64 duration) {
    TDirective directive;
    auto& setTimerDirective = *directive.MutableSetTimerDirective();
    setTimerDirective.SetName("set_timer");
    setTimerDirective.SetDuration(duration);
    setTimerDirective.SetListeningIsPossible(false);

    auto& playerPauseDirective = *setTimerDirective.AddDirectives()->MutablePlayerPauseDirective();
    playerPauseDirective.SetName("player_pause");
    playerPauseDirective.SetSmooth(true);
    setTimerDirective.AddDirectives()->MutableClearQueueDirective()->SetName("clear_queue");
    setTimerDirective.AddDirectives()->MutableGoHomeDirective()->SetName("go_home");
    setTimerDirective.AddDirectives()->MutableScreenOffDirective()->SetName("screen_off");

    AddDirective(std::move(directive));
}

void TResponseBodyBuilder::AddDirective(TDirective&& directive) {
    *ResponseBody_.MutableLayout()->AddDirectives() = std::move(directive);
}

void TResponseBodyBuilder::AddServerDirective(TServerDirective&& directive) {
    *ResponseBody_.AddServerDirectives() = std::move(directive);
}

bool TResponseBodyBuilder::TryAddMementoUserConfig(
    const ru::yandex::alice::memento::proto::EConfigKey key,
    const NProtoBuf::Message& value)
{
    ru::yandex::alice::memento::proto::TConfigKeyAnyPair config;
    config.SetKey(key);
    if (!config.MutableValue()->PackFrom(value)) {
        return false;
    }

    TServerDirective directive;
    *directive.MutableMementoChangeUserObjectsDirective()->MutableUserObjects()->AddUserConfigs() = std::move(config);
    AddServerDirective(std::move(directive));

    return true;
}

void TResponseBodyBuilder::AddRenderedSuggest(TResponseBodyBuilder::TSuggest&& suggest) {
    TString actionId;
    for (auto& action : suggest.Directives) {
        AddAction(std::move(action), actionId);
    }

    if (suggest.AutoDirective) {
        *ResponseBody_.MutableLayout()->AddDirectives() = std::move(*suggest.AutoDirective);
    }

    if (suggest.ButtonForText) {
        NScenarios::TLayout::TButton button;
        button.SetTitle(std::move(*suggest.ButtonForText));
        button.SetActionId(actionId);
        ButtonsForText_.push_back(std::move(button));
    }

    if (suggest.SuggestButton) {
        AddActionSuggest(actionId).Title(*suggest.SuggestButton);
    }
}

TActionButtonBuilder TResponseBodyBuilder::AddActionSuggest(const TString& actionId) {
    return TActionButtonBuilder{*ResponseBody_.MutableLayout()->AddSuggestButtons()->MutableActionButton(), actionId};
}

TSearchButtonBuilder TResponseBodyBuilder::AddSearchSuggest() {
    return TSearchButtonBuilder{*ResponseBody_.MutableLayout()->AddSuggestButtons()->MutableSearchButton()};
}

TResetAddBuilder TResponseBodyBuilder::ResetAddBuilder() {
    return TResetAddBuilder{*ResponseBody_.MutableStackEngine()->AddActions()->MutableResetAdd()};
}

void TResponseBodyBuilder::AddNewSessionStackAction() {
    *ResponseBody_.MutableStackEngine()->AddActions()->MutableNewSession() = {};
}

void TResponseBodyBuilder::SetExpectsRequest(bool expectsRequest) {
    ResponseBody_.SetExpectsRequest(expectsRequest);
}

void TResponseBodyBuilder::SetShouldListen(bool shouldListen) {
    ResponseBody_.MutableLayout()->SetShouldListen(shouldListen);
}

void TResponseBodyBuilder::SetState(const ::google::protobuf::Message& state) {
    ResponseBody_.MutableState()->PackFrom(state);
}

void TResponseBodyBuilder::AddTypeTextSuggest(const TString& text, const TMaybe<TString>& typeText, const TMaybe<TString>& name) {
    TResponseBodyBuilder::TSuggest suggest;

    TDirective directive;
    TTypeTextDirective* typeTextDirective = directive.MutableTypeTextDirective();
    if (name.Defined()) {
        typeTextDirective->SetName(*name);
    }
    if (typeText.Defined()) {
        typeTextDirective->SetText(*typeText);
    } else {
        typeTextDirective->SetText(text);
    }
    suggest.Directives.push_back(directive);

    suggest.SuggestButton = text;
    AddRenderedSuggest(std::move(suggest));
}

void TResponseBodyBuilder::AddWidgetRenderData(NData::TScenarioData& scenarioData) {
    const auto& cardItems = scenarioData.GetCentaurWidgetCardItemData().GetCentaurWidgetCardItems();
    for (const auto& cardItem : cardItems) {
        NRenderer::TDivRenderData renderData;
        renderData.MutableScenarioData()->MutableCentaurWidgetCardItem()->CopyFrom(cardItem);

        const auto cardId = cardItem.GetCardData().GetId();
        renderData.SetCardId(cardId);
        RenderData_[cardId] = renderData;

        const auto cardIdCompact = cardId + "_compact";
        renderData.SetCardId(cardIdCompact);
        RenderData_[cardIdCompact] = renderData;
    }
}

void TResponseBodyBuilder::AddTtsPlayPlaceholderDirective(const NJson::TJsonValue& value /* = NJson::TJsonValue()*/)
{
    // TODO [DD] Response may have only one directive, probably its need to cause an exception in case if scenario is trying to add directive twice
    // To be discussed(?)
    AddClientActionDirective("tts_play_placeholder", value);
}

void TResponseBodyBuilder::AddOpenUriDirective(const TString& uri, const TStringBuf screen_id, const TStringBuf name) {
    Y_ENSURE(!uri.empty(), "open_uri directive requies a non-empty uri");
    NAlice::NScenarios::TDirective directive;
    directive.MutableOpenUriDirective()->SetUri(uri);
    if (!screen_id.empty()) {
        directive.MutableOpenUriDirective()->SetScreenId(screen_id.data(), screen_id.size());
    }
    if (!name.empty()) {
        directive.MutableOpenUriDirective()->SetName(name.data());
    }
    AddDirective(std::move(directive));
}

void TResponseBodyBuilder::AddShowPromoDirective() {
    
    NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder = GetOrCreateAnalyticsInfoBuilder();
    NScenarios::TAnalyticsInfo::TObject source;
    source.MutableDummyResponse()->SetReason(NAlice::NAnalytics::NDummyResponse::TResponse_EReason_SurfaceInability);
    analyticsInfoBuilder.AddObject(source);

    NAlice::NScenarios::TDirective directive;
    Y_UNUSED(directive.MutableShowPromoDirective());
    AddDirective(std::move(directive));
}

bool TResponseBodyBuilder::TryAddAuthorizationDirective(const bool supportsOpenYandexAuth) {
    if (const auto authorizationUrl = GenerateAuthorizationUri(supportsOpenYandexAuth)) {
        AddOpenUriDirective(authorizationUrl);
        return true;
    }
    return false;
}

bool TResponseBodyBuilder::TryAddTtsPlayPlaceholderDirective() {
    constexpr auto isTtsPlayPlaceholder = [](auto& d) { return d.HasTtsPlayPlaceholderDirective(); };
    if (AnyOf(ResponseBody_.GetLayout().GetDirectives(), isTtsPlayPlaceholder)) {
        return false;
    }
    TDirective d;
    d.MutableTtsPlayPlaceholderDirective();
    AddDirective(std::move(d));
    return true;
}

void TResponseBodyBuilder::AddListenDirective(TMaybe<int> silenceTimeoutMs) {
    TryAddTtsPlayPlaceholderDirective();
    TDirective d;
    d.MutableListenDirective();
    if (silenceTimeoutMs.Defined()) {
        d.MutableListenDirective()->SetStartingSilenceTimeoutMs(silenceTimeoutMs.GetRef());
    }
    AddDirective(std::move(d));
}

bool TResponseBodyBuilder::HasListenDirective() const {
    constexpr auto isListenDirective  = [](auto& d) { return d.HasListenDirective(); };
    return AnyOf(ResponseBody_.GetLayout().GetDirectives(), isListenDirective);
}

void TResponseBodyBuilder::AddWebViewMediaSessionPlayDirective(const TString& mediaSessionId) {
    TDirective directive;
    auto& webViewMediaSessionPlayDirective = *directive.MutableWebViewMediaSessionPlayDirective();
    webViewMediaSessionPlayDirective.SetName("web_view_media_session_play");
    webViewMediaSessionPlayDirective.SetMediaSessionId(mediaSessionId);
    AddDirective(std::move(directive));
}

void TResponseBodyBuilder::AddWebViewMediaSessionPauseDirective(const TString& mediaSessionId) {
    TDirective directive;
    auto& webViewMediaSessionPauseDirective = *directive.MutableWebViewMediaSessionPauseDirective();
    webViewMediaSessionPauseDirective.SetName("web_view_media_session_pause");
    webViewMediaSessionPauseDirective.SetMediaSessionId(mediaSessionId);
    AddDirective(std::move(directive));
}

void TResponseBodyBuilder::AddRenderedText(const TString& text) {
    Renderer_->AddRenderedText(ResponseBody_, text);
}

void TResponseBodyBuilder::AddRenderedVoice(const TString& voice) {
    Renderer_->AddRenderedVoice(ResponseBody_, voice);
}

void TResponseBodyBuilder::AddRenderedTextWithButtons(const TString& text,
                                                      const TVector<TLayout::TButton>& buttons)
{
    TVector<TLayout::TButton> expandedButtons = buttons;
    for (const auto& button : ButtonsForText_) {
        expandedButtons.push_back(button);
    }
    Renderer_->AddRenderedTextWithButtons(ResponseBody_, text, expandedButtons);
}

void TResponseBodyBuilder::Build() {
    if (Nlg_) {
        if (const auto& records = Nlg_->GetNlgRenderHistoryRecords()) {
            ResponseBody_.MutableAnalyticsInfo()->MutableNlgRenderHistoryRecords()->Add(records.cbegin(), records.cend());
        }
        if (ResponseBody_.GetContextualData().GetResponseLanguage() == ::NAlice::ELang::L_UNK) {
            if (const auto nlgLang = Nlg_->GetLang(); nlgLang != ELanguage::LANG_RUS) { // crutch not to recanonize all it2 tests
                ResponseBody_.MutableContextualData()->SetResponseLanguage(static_cast<::NAlice::ELang>(Nlg_->GetLang()));
            }
        }
    }
}

} // namespace NAlice::NHollywood
