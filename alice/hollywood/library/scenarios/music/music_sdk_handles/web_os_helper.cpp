#include "web_os_helper.h"

#include <alice/hollywood/library/environment_state/endpoint.h>
#include <alice/hollywood/library/nlg/nlg_data.h>
#include <alice/hollywood/library/scenarios/music/analytics_info/analytics_info.h>
#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/music/defs.h>
#include <alice/library/url_builder/url_builder.h>

#include <alice/protos/endpoint/capability.pb.h>
#include <alice/protos/endpoint/endpoint.pb.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

namespace {

constexpr TStringBuf KINOPOISK_APP_ID = "tv.kinopoisk.ru";
constexpr TStringBuf YOUTUBE_APP_ID = "youtube.leanback.v4";

struct TWebOSLaunchAppPayload {
    TString AppId;
    NJson::TJsonValue Params;
};

TMaybe<TWebOSCapability> GetWebOSCapability(const TEnvironmentState* envState) {
    if (envState) {
        const TEndpoint* webOSEndpoint = FindEndpoint(*envState, TEndpoint_EEndpointType_WebOsTvEndpointType);
        if (webOSEndpoint) {
            TWebOSCapability webOSCapability;
            if (NHollywood::ParseTypedCapability(webOSCapability, *webOSEndpoint)) {
                return webOSCapability;
            }
        }
    }

    return Nothing();
}

bool HasYoutubeInForeground(const TEnvironmentState* envState) {
    const auto webOSCapability = GetWebOSCapability(envState);
    if (webOSCapability.Defined()) {
        return webOSCapability->GetState().GetForegroundAppId() == YOUTUBE_APP_ID;
    }

    return false;
}

TWebOSLaunchAppPayload ConstructWebOSLaunchAppPayloadForYoutube(const TScenarioBaseRequestWithInputWrapper& request) {
    TWebOSLaunchAppPayload youtubePayload;
    youtubePayload.AppId = YOUTUBE_APP_ID;
    TCgiParameters deeplinkParams;
    deeplinkParams.InsertUnescaped("inApp", TStringBuf("true"));
    deeplinkParams.InsertUnescaped("launch_tag", TStringBuf("voice"));
    deeplinkParams.InsertUnescaped("vq", request.Input().Utterance());
    youtubePayload.Params["contentTarget"] = deeplinkParams.Print();
    return youtubePayload;
}

TWebOSLaunchAppPayload ConstructPayloadForWebOS(
    const TScenarioBaseRequestWithInputWrapper& request, TNlgDataBuilder& nlgDataBuilder,
    bool isUserUnauthorizedOrWithoutSubscription,
    const TMaybe<TContentId>& contentId, const NAlice::TEnvironmentState* envState)
{
    if (HasYoutubeInForeground(envState)) {
        nlgDataBuilder.AddAttention("launch_youtube_app");
        return ConstructWebOSLaunchAppPayloadForYoutube(request);
    } else if (isUserUnauthorizedOrWithoutSubscription) {
        TWebOSLaunchAppPayload kpPayload;
        kpPayload.AppId = KINOPOISK_APP_ID;
        auto& params = kpPayload.Params;
        params["pageId"] = "music";
        return kpPayload;
    } else {
        const auto frameProto = request.Input().FindSemanticFrame(MUSIC_PLAY_FRAME);
        Y_ENSURE(frameProto);
        auto musicPlayFrame = TFrame::FromProto(*frameProto);

        Y_ENSURE(contentId);
        TWebOSLaunchAppPayload kpPayload;
        kpPayload.AppId = KINOPOISK_APP_ID;
        auto& params = kpPayload.Params;
        params["pageId"] = "musicPlayer";
        params["type"] = ContentTypeToText(contentId->GetType());
        if (NeedShuffle(musicPlayFrame)) {
            params["shuffle"] = true;
        }
        if (GetRepeatMode(musicPlayFrame) == TMusicSdkUriBuilder::ERepeatMode::REPEAT_ALL) {
            params["repeat"] = true;
        }

        if (contentId->GetType() == TContentId_EContentType_Track ||
            contentId->GetType() == TContentId_EContentType_Album ||
            contentId->GetType() == TContentId_EContentType_Artist)
        {
            params["id"] = contentId->GetId();
        } else if (contentId->GetType() == TContentId_EContentType_Playlist) {
            auto playlistId = TPlaylistId::FromString(contentId->GetId());
            params["owner"] = playlistId->Owner;
            params["kind"] = playlistId->Kind;
        } else if (contentId->GetType() == TContentId_EContentType_Radio) {
            params["tag"] = contentId->GetId();
        }

        return kpPayload;
    }
}

} // namespace

void AddWebOSLaunchAppDirective(
    const TScenarioBaseRequestWithInputWrapper& request, TNlgDataBuilder& nlgDataBuilder, TResponseBodyBuilder& bodyBuilder,
    bool isUserUnauthorizedOrWithoutSubscription, const TMaybe<TContentId>& contentId, const NAlice::TEnvironmentState* envState)
{
    const auto payload = ConstructPayloadForWebOS(request, nlgDataBuilder,
        isUserUnauthorizedOrWithoutSubscription, contentId, envState);
    NScenarios::TDirective directive;
    auto& webOSLaunchAppDirective = *directive.MutableWebOSLaunchAppDirective();
    webOSLaunchAppDirective.SetName("web_os_launch_app");
    webOSLaunchAppDirective.SetAppId(payload.AppId);
    webOSLaunchAppDirective.SetParamsJson(JsonToString(payload.Params));
    bodyBuilder.AddDirective(std::move(directive));
}

} // namespace NAlice::NHollywood::NMusic::NMusicSdk
