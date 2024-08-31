#include "common.h"
#include "web_os_helper.h"

#include <alice/hollywood/library/nlg/nlg_data.h>
#include <alice/hollywood/library/scenarios/music/analytics_info/analytics_info.h>
#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/music_sdk_handles/nlg_data_builder/nlg_data_builder.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/music/defs.h>
#include <alice/library/url_builder/url_builder.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

namespace {

// TODO(sparkle): fake flag, remove after successful release of navigator MusicSDK
constexpr TStringBuf EXP_HW_MUSIC_SDK_CLIENT_DISABLE_NAVIGATOR = "hw_music_sdk_client_disable_navigator";
constexpr TStringBuf EXP_HW_MUSIC_SDK_CLIENT_DISABLE_SEARCH_APP = "hw_music_sdk_client_disable_search_app";

constexpr TStringBuf ORDER_SLOT_SHUFFLE = "shuffle";
constexpr TStringBuf REPEAT_SLOT_REPEAT = "repeat";

constexpr TStringBuf VERTICAL_URI = "https://music.yandex.ru/pptouch";

TString ConstructOnboardingCaption(TNlgWrapper& nlg, const TNlgData& nlgData) {
    const auto caption = nlg.RenderPhrase(
        /* nlgTemplateName = */ TEMPLATE_MUSIC_PLAY,
        /* phraseName = */ "render_suggest_caption__onboarding__what_can_you_do",
        /* nlgData = */ nlgData
    ).Text;
    return caption;
}

bool SupportsMusicSDKPlayer(const TScenarioRunRequestWrapper& request) {
    // FIXME(sparkle): use "request.Interfaces().GetHasMusicSdkClient()" instead of "request.ClientFeatures().SupportsMusicSDKPlayer()"
    // after HOLLYWOOD-711 Megamind part released
    return request.Interfaces().GetHasMusicSdkClient();
}

} // namespace

const TString SUBGRAPH_APPHOST_FLAG = "music_sdk_subgraph";
const TString PLAYLIST_SETDOWN_RESPONSE_FAILED_APPHOST_FLAG = "playlist_setdown_response_failed";

bool ShouldInvokeSubgraph(const TScenarioRunRequestWrapper& request) {
    return request.Input().FindSemanticFrame(MUSIC_PLAY_FRAME) &&
           SupportsMusicSDKPlayer(request) &&
           (IsSearchAppSupported(request) || IsNavigatorSupported(request) || request.ClientInfo().IsLegatus());
}

bool IsRadioSupported(const TScenarioBaseRequestWrapper& request) {
    return request.ClientInfo().IsNavigator() || request.ClientInfo().IsLegatus();
}

bool IsNavigatorSupported(const TScenarioBaseRequestWrapper& request) {
    return !request.HasExpFlag(EXP_HW_MUSIC_SDK_CLIENT_DISABLE_NAVIGATOR) &&
           request.ClientInfo().IsNavigator();
}

bool IsSearchAppSupported(const TScenarioBaseRequestWrapper& request) {
    return !request.HasExpFlag(EXP_HW_MUSIC_SDK_CLIENT_DISABLE_SEARCH_APP) &&
           request.ClientInfo().IsSearchApp();
}

bool CanRenderDiv2Cards(const TScenarioBaseRequestWrapper& request) {
    // searchapp supports div2 cards without flags
    return request.ClientInfo().IsSearchApp() || request.Interfaces().GetCanRenderDiv2Cards();
}

bool IsUserAuthorized(const NScenarios::TRequestMeta& meta) {
    return !meta.GetOAuthToken().Empty();
}

bool UserHasSubscription(const TScenarioRunRequestWrapper& request) {
    const auto* userInfo = GetUserInfoProto(request);
    return userInfo && userInfo->GetHasMusicSubscription();
}

NScenarios::TScenarioRunResponse CreateErrorResponse(
    TRTLogger& logger,
    const TScenarioRunRequestWrapper& request,
    TNlgWrapper& nlg,
    const TString& errorType,
    const TString& errorCode,
    const bool setIrrelevant)
{
    // should have been THwFrameworkRunResponseBuilder, but that would make no sense
    TRunResponseBuilder response(&nlg);
    if (setIrrelevant) {
        response.SetIrrelevant();
    }

    auto& bodyBuilder = response.CreateResponseBodyBuilder();
    TNlgDataBuilder nlgDataBuilder{logger, request};
    nlgDataBuilder.SetErrorCode(errorCode);
    if (request.ClientInfo().IsLegatus()) {
        AddWebOSLaunchAppDirective(request, nlgDataBuilder, bodyBuilder,
            /* isUserUnauthorizedOrWithoutSubscription = */ true, Nothing(), GetEnvironmentStateProto(request));
    }
    const auto phraseName = TString::Join("render_error__", errorType);
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_MUSIC_PLAY, phraseName,
                                                   /* buttons = */ {}, nlgDataBuilder.GetNlgData());

    // Imitate VINS error meta
    auto& analyticsInfoBuilder = bodyBuilder.CreateAnalyticsInfoBuilder();
    FillAnalyticsInfoForMusicPlaySimple(analyticsInfoBuilder);
    FillAnalyticsInfoVinsErrorMeta(analyticsInfoBuilder, request, errorType);

    return *std::move(response).BuildResponse();
}

NScenarios::TScenarioRunResponse CreateIrrelevantResponse(
    TRTLogger& logger,
    const TScenarioRunRequestWrapper& request,
    TNlgWrapper& nlg)
{
    return CreateErrorResponse(logger, request, nlg, /* errorType = */ "musicerror",
                               /* errorCode = */ "music_not_found", /* setIrrelevant = */ true);
}

NScenarios::TScenarioRunResponse CreateYouNeedAuthorizeResponse(
    TRTLogger& logger,
    const TScenarioRunRequestWrapper& request,
    TNlgWrapper& nlg)
{
    // should have been THwFrameworkRunResponseBuilder, but that would make no sense
    TRunResponseBuilder response(&nlg);
    auto& bodyBuilder = response.CreateResponseBodyBuilder();
    TNlgData nlgData{logger, request};
    nlgData.AddAttention("unauthorized");

    TString uri = GenerateAuthorizationUri(request.Interfaces().GetCanOpenYandexAuth());
    Y_ENSURE(!uri.Empty());
    nlgData.Context["authorize"]["data"]["uri"] = std::move(uri);

    AddOpenUriSuggest(bodyBuilder, nlg, nlgData, "authorize", "render_buttons_open_uri");
    TryAddSearchSuggest(request.ClientInfo(), request.Input(), bodyBuilder);
    AddOnboardingSuggest(bodyBuilder, nlg, nlgData);

    auto& analyticsInfoBuilder = bodyBuilder.CreateAnalyticsInfoBuilder();
    FillAnalyticsInfoForMusicPlaySimple(analyticsInfoBuilder);

    return *std::move(response).BuildResponse();
}

// old code: https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/music/quasar_provider.cpp?rev=r8885588#L1221
template<typename TResponse>
TResponse ConstructFallbackToMusicVerticalResponse(
    TRTLogger& logger,
    const TScenarioBaseRequestWithInputWrapper& request,
    TNlgWrapper& nlg,
    const TFrame& musicPlayFrame,
    const bool isGeneral)
{
    TString musicVerticalUri{VERTICAL_URI};

    using TResponseBuilder = std::conditional_t<
        std::is_same_v<TResponse, NScenarios::TScenarioRunResponse>,
        // should have been THwFrameworkRunResponseBuilder, but that would make no sense
        TRunResponseBuilder,
        TContinueResponseBuilder
    >;
    TResponseBuilder response(&nlg);
    auto& bodyBuilder = response.CreateResponseBodyBuilder();
    TNlgDataBuilder nlgDataBuilder{logger, request, musicPlayFrame};

    // build text cards and directives
    nlgDataBuilder.AddMusicVerticalUri(musicVerticalUri);
    if (isGeneral) {
        nlgDataBuilder.AddAttention("fallback_to_music_vertical_general");
    } else {
        nlgDataBuilder.AddAttention("fallback_to_music_vertical_nothing_found");
    }
    AddOpenUriDirective(bodyBuilder, std::move(musicVerticalUri), /* name= */ "music_vertical_show");
    AddOpenUriSuggest(bodyBuilder, nlg, nlgDataBuilder.GetNlgData(),
                      /* type= */ "fallback_to_music_vertical",
                      /* directiveName= */ "render_buttons_open_uri");

    // build suggests
    TryAddSearchSuggest(request.ClientInfo(), request.Input(), bodyBuilder);
    AddOnboardingSuggest(bodyBuilder, nlg, nlgDataBuilder.GetNlgData());

    // build analytics
    auto& analyticsInfoBuilder = bodyBuilder.CreateAnalyticsInfoBuilder();
    FillAnalyticsInfoForMusicPlaySimple(analyticsInfoBuilder);
    FillAnalyticsInfoSelectedSourceEvent(analyticsInfoBuilder, request);
    if constexpr (requires { response.SetFeaturesIntent; }) {
        response.SetFeaturesIntent(musicPlayFrame.Name());
    }

    return *std::move(response).BuildResponse();
}

// nasty hack for linking methods
template NScenarios::TScenarioRunResponse ConstructFallbackToMusicVerticalResponse<NScenarios::TScenarioRunResponse>
(TRTLogger& logger, const TScenarioBaseRequestWithInputWrapper& request, TNlgWrapper& nlg, const TFrame& musicPlayFrame, const bool isGeneral);

template NScenarios::TScenarioContinueResponse ConstructFallbackToMusicVerticalResponse<NScenarios::TScenarioContinueResponse>
(TRTLogger& logger, const TScenarioBaseRequestWithInputWrapper& request, TNlgWrapper& nlg, const TFrame& musicPlayFrame, const bool isGeneral);


void AddOpenUriSuggest(TResponseBodyBuilder& bodyBuilder, TNlgWrapper& nlg, const TNlgData& nlgData,
                       const TStringBuf type, const TStringBuf directiveName)
{
    const TString actionId = "open_uri_action_id";
    TString suggestUri = nlg.RenderPhrase(TEMPLATE_MUSIC_PLAY, TString::Join("render_suggest_uri__", type), nlgData).Text;
    TString suggestCaption = nlg.RenderPhrase(TEMPLATE_MUSIC_PLAY, TString::Join("render_suggest_caption__", type), nlgData).Text;

    NScenarios::TFrameAction action;

    NScenarios::TDirective directive;
    auto& openUriDirective = *directive.MutableOpenUriDirective();
    openUriDirective.SetUri(std::move(suggestUri));
    openUriDirective.SetName(directiveName.data(), directiveName.size());
    *action.MutableDirectives()->AddList() = std::move(directive);

    bodyBuilder.AddAction(actionId, std::move(action));

    NScenarios::TLayout::TButton button;
    button.SetTitle(std::move(suggestCaption));
    button.SetActionId(std::move(actionId));

    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_MUSIC_PLAY, "render_result",
                                                   /* buttons= */ {std::move(button)},
                                                   /* nlgData= */ nlgData);
}

void AddOpenUriDirective(TResponseBodyBuilder& bodyBuilder, TString uri, const TStringBuf name) {
    NScenarios::TDirective directive;
    auto& openUriDirective = *directive.MutableOpenUriDirective();
    openUriDirective.SetName(name.data(), name.size());
    openUriDirective.SetUri(std::move(uri));
    bodyBuilder.AddDirective(std::move(directive));
}

void TryAddSearchSuggest(const TClientInfo& clientInfo, const TScenarioInputWrapper& input,
                         TResponseBodyBuilder& bodyBuilder)
{
    // old code: https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/context/context.cpp?rev=r8701492#L304

    // do not add search suggests to Navigator, Ya.Auto, Quasar (Ya.Station)
    if (clientInfo.IsNavigator() || clientInfo.IsYaAuto() || clientInfo.IsSmartSpeaker()) {
        return;
    }

    if (const TString utterance = input.Utterance(); !utterance.Empty()) {
        bodyBuilder.AddSearchSuggest().Title(utterance).Query(utterance);
    }
}

void AddOnboardingSuggest(TResponseBodyBuilder& bodyBuilder, TNlgWrapper& nlg, const TNlgData& nlgData) {
    const auto caption = ConstructOnboardingCaption(nlg, nlgData);
    bodyBuilder.AddTypeTextSuggest(/* text= */ caption, /* typeText = */ caption, /* name = */ "render_buttons_type");
}

bool IsVariousArtistsCase(const NJson::TJsonValue& bassState) {
    const auto& artists = bassState["apply_arguments"]["web_answer"]["artists"].GetArray();
    if (artists.empty()) {
        return false;
    }
    return artists[0]["is_various"].GetBooleanSafe(/* defaultValue= */ false);
}

bool NeedShuffle(const TFrame& musicPlayFrame) {
    // old logic:
    // BASS - https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/music/quasar_provider.cpp?rev=r8637801#L525
    // MusicQuasar - https://a.yandex-team.ru/arc/trunk/arcadia/music/backend/music-web/src/main/java/ru/yandex/music/support/quasar/bass/BassActionContainer.java?rev=r8637706#L225
    if (const auto orderSlot = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_ORDER)) {
        return orderSlot->Value.AsString() == ORDER_SLOT_SHUFFLE;
    }
    return false;
}

TMusicSdkUriBuilder::ERepeatMode GetRepeatMode(const TFrame& musicPlayFrame) {
    // old logic:
    // BASS - https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/music/quasar_provider.cpp?rev=r8637801#L526
    // MusicQuasar - https://a.yandex-team.ru/arc/trunk/arcadia/music/backend/music-web/src/main/java/ru/yandex/music/support/quasar/bass/BassActionContainer.java?rev=r8637706#L226-229
    if (const auto repeatSlot = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_REPEAT)) {
        if (repeatSlot->Value.AsString() == REPEAT_SLOT_REPEAT) {
            return TMusicSdkUriBuilder::ERepeatMode::REPEAT_ALL;
        }
    }
    return TMusicSdkUriBuilder::ERepeatMode::REPEAT_OFF;
}

namespace NUsualPlaylist {

TString FindOwnerLogin(const NJson::TJsonValue& playlistObj) {
    Y_ENSURE(playlistObj.Has("result"));
    return playlistObj["result"]["playlists"]["results"][0]["owner"]["login"].GetStringRobust();
}

TString FindOwnerUid(const NJson::TJsonValue& playlistObj) {
    Y_ENSURE(playlistObj.Has("result"));
    return playlistObj["result"]["playlists"]["results"][0]["owner"]["uid"].GetStringRobust();
}

bool FindOwnerIsVerified(const NJson::TJsonValue& playlistObj) {
    Y_ENSURE(playlistObj.Has("result"));
    return playlistObj["result"]["playlists"]["results"][0]["owner"]["verified"].GetBooleanRobust();
}

TString FindPlaylistId(const NJson::TJsonValue& playlistObj) {
    Y_ENSURE(playlistObj.Has("result"));
    const auto& firstResult = playlistObj["result"]["playlists"]["results"][0];
    return TString::Join(firstResult["owner"]["uid"].GetStringRobust(), ":", firstResult["kind"].GetStringRobust());
}

} // namespace NUsualPlaylist

namespace NSpecialPlaylist {

TString FindOwnerLogin(const NJson::TJsonValue& playlistObj) {
    Y_ENSURE(playlistObj.Has("result"));
    return playlistObj["result"]["data"]["owner"]["login"].GetStringRobust();
}

TString FindOwnerUid(const NJson::TJsonValue& playlistObj) {
    Y_ENSURE(playlistObj.Has("result"));
    return playlistObj["result"]["data"]["owner"]["uid"].GetStringRobust();
}

bool FindOwnerIsVerified(const NJson::TJsonValue& playlistObj) {
    Y_ENSURE(playlistObj.Has("result"));
    return playlistObj["result"]["data"]["owner"]["verified"].GetBooleanRobust();
}

TMaybe<TString> FindPlaylistId(const NJson::TJsonValue& playlistObj) {
    Y_ENSURE(playlistObj.Has("result"));
    if (const auto* ready = playlistObj.GetValueByPath("result.ready")) {
        if (!ready->GetBoolean()) {
            return Nothing();
        }
    }
    const auto& result = playlistObj["result"]["data"];
    return TString::Join(result["owner"]["uid"].GetStringRobust(), ":", result["kind"].GetStringRobust());
}

} // namespace NSpecialPlaylist

namespace NPredefinedPlaylist {

TString FindOwnerLogin(const NJson::TJsonValue& playlistObj) {
    Y_ENSURE(playlistObj.Has("result"));
    return playlistObj["result"]["owner"]["login"].GetStringRobust();
}

TString FindOwnerUid(const NJson::TJsonValue& playlistObj) {
    Y_ENSURE(playlistObj.Has("result"));
    return playlistObj["result"]["owner"]["uid"].GetStringRobust();
}

bool FindOwnerIsVerified(const NJson::TJsonValue& playlistObj) {
    Y_ENSURE(playlistObj.Has("result"));
    return playlistObj["result"]["owner"]["verified"].GetBooleanRobust();
}

} // namespace NPredefinedPlaylist

} // namespace NAlice::NHollywood::NMusic::NMusicSdk
