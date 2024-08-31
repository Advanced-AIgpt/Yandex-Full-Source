#include "web_os_helper.h"
#include "environment_state.h"

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/directives.h>
#include <alice/library/video_common/defs.h>
#include <alice/library/util/system_time.h>

#include <alice/megamind/protos/common/environment_state.pb.h>
#include <alice/protos/endpoint/capability.pb.h>
#include <alice/protos/endpoint/endpoint.pb.h>

#include <google/protobuf/util/json_util.h>

namespace NBASS::NVideo {

namespace {

constexpr TStringBuf KINOPOISK_APP_ID = "tv.kinopoisk.ru";
constexpr TStringBuf YOUTUBE_APP_ID = "youtube.leanback.v4";

const NAlice::TEndpoint* FindEndpoint(const NAlice::TEnvironmentState& environmentState, NAlice::TEndpoint_EEndpointType type) {
    return FindIfPtr(environmentState.GetEndpoints(), [type](const auto& endpoint) {
        return endpoint.GetMeta().GetType() == type;
    });
}

TMaybe<NAlice::TWebOSCapability> GetWebOSCapability(TContext& ctx) {
    TEnvironmentStateHelper environmentStateHelper{ctx};
    const auto environmentState = environmentStateHelper.GetEnvironmentState();
    if (environmentState.Defined()) {
        const NAlice::TEndpoint* webOSEndpoint = FindEndpoint(*environmentState, NAlice::TEndpoint_EEndpointType_WebOsTvEndpointType);
        if (webOSEndpoint) {
            for (const auto& capability : webOSEndpoint->GetCapabilities()) {
                NAlice::TWebOSCapability webOSCapability;
                if (capability.UnpackTo(&webOSCapability)) {
                    return webOSCapability;
                }
            };
        }
    }

    return Nothing();
}

bool HasYoutubeInForeground(TContext& ctx) {
    const auto webOSCapability = GetWebOSCapability(ctx);
    if (webOSCapability.Defined()) {
        return webOSCapability.GetRef().GetState().GetForegroundAppId() == YOUTUBE_APP_ID;
    }

    return false;
}

bool TryAddYoutubeWebOSLaunchAppCommand(TContext& ctx) {
    if (!HasYoutubeInForeground(ctx)) {
        return false;
    }

    NSc::TValue youtubePayload;
    youtubePayload["appId"] = YOUTUBE_APP_ID;
    TCgiParameters deeplinkParams;
    deeplinkParams.InsertUnescaped("inApp", TStringBuf("true"));
    deeplinkParams.InsertUnescaped("launch_tag", TStringBuf("voice"));
    deeplinkParams.InsertUnescaped("vq", ctx.Meta().Utterance());
    youtubePayload["params"]["contentTarget"] = deeplinkParams.Print();
    ctx.AddCommand<TWebOSLaunchAppDirective>(NAlice::NVideoCommon::COMMAND_WEB_OS_LAUNCH_APP, youtubePayload);
    ctx.AddAttention("launch_youtube_app");
    return true;
}

void TryUpdateInfoIfNeeded(TContext& ctx, TVideoGallery& gallery) {
    TVector<TString> kpIds;
    for (const auto& videoItem : gallery->Items()) {
        if (videoItem->VhLicenses()->HorizontalPoster()->Empty()) {
            kpIds.push_back(TString{videoItem->MiscIds().Kinopoisk()});
        }
    }
    if (kpIds.empty()) {
        return;
    }

    NHttpFetcher::TRequestPtr request = ctx.GetSources().VideoLsOtt("/ya-station/v1/search/content-tree").Request();
    TPersonalDataHelper personalDataHelper(ctx);
    request->AddCgiParam("countryId", ToString(ctx.GlobalCtx().GeobaseLookup().GetCountryId(ctx.UserRegion())));
    TString puid;
    if (personalDataHelper.GetUid(puid)) {
        request->AddCgiParam("puid", puid);
    }
    request->AddCgiParam("shallowMode", "true");
    request->AddCgiParam("withStreams", "false");
    request->AddCgiParam("withTrackings", "false");
    request->AddCgiParam("isPublicProxy", "false");
    request->AddCgiParam("isAnonymousVpn", "false");
    request->AddCgiParam("userAgent", ctx.MetaClientInfo().UserAgent);
    for (const auto& kpId : kpIds) {
        request->AddCgiParam("kpId", kpId);
    }

    NHttpFetcher::THandle::TRef handler = request->Fetch();
    if (!handler) {
        return;
    }

    const auto startMillis = NAlice::SystemTimeNowMillis();
    NHttpFetcher::TResponse::TConstRef resp = handler->Wait();
    ctx.AddStatsCounter("LsOtt_milliseconds", NAlice::SystemTimeNowMillis() - startMillis);

    NSc::TValue json;
    if (resp->IsError() || resp->Data.empty() || !NSc::TValue::FromJson(json, resp->Data)) {
        return;
    }

    THashMap<TString, TString> kpidToHorizontalPoster;
    THashMap<TString, NSc::TValue> kpidToLicenses;
    for (const auto& kpItem : json["content_groups"].GetArray()) {
        kpidToHorizontalPoster[ToString(kpItem["kinopoisk_id"])] = kpItem["horizontal_poster"];
        NSc::TValue licenses;
        for (const auto& license : kpItem["licenses"].GetArray()) {
            if (license["monetization_model"] == "SVOD") {
                licenses["svod"]["subscriptions"].Push(license["subscription"].GetString());
            } else if (license["monetization_model"] == "EST") {
                licenses["est"]["discount_price"] = license["price_with_discount"];
                licenses["est"]["price"] = license["price"];
            } else if (license["monetization_model"] == "TVOD") {
                licenses["tvod"]["discount_price"] = license["price_with_discount"];
                licenses["tvod"]["price"] = license["price"];
            }
        }
        kpidToLicenses[ToString(kpItem["kinopoisk_id"])] = licenses;
    }

    for (size_t i = 0; i < gallery->Items().Size(); i++) {
        auto videoItem = gallery->Items(i);
        if (videoItem->VhLicenses()->HorizontalPoster()->Empty() && kpidToHorizontalPoster.contains(videoItem->MiscIds().Kinopoisk())) {
            LOG(INFO) << "Found horizontal poster from lsOtt response" << Endl;
            videoItem->VhLicenses()->HorizontalPoster() = kpidToHorizontalPoster[videoItem->MiscIds().Kinopoisk()];
            videoItem->VhLicenses()->Est() = kpidToLicenses[videoItem->MiscIds().Kinopoisk()]["est"].ToJson();
            videoItem->VhLicenses()->Tvod() = kpidToLicenses[videoItem->MiscIds().Kinopoisk()]["tvod"].ToJson();
            videoItem->VhLicenses()->Svod() = kpidToLicenses[videoItem->MiscIds().Kinopoisk()]["svod"].ToJson();
        }
    }
}

} // namespace

void AddWebOSLaunchAppCommandForOnboarding(TContext& ctx) {
    NSc::TValue onboardingPayload;
    onboardingPayload["appId"] = "yandex.alice";
    auto& params = onboardingPayload["params"];
    params["request"] = "launch";
    params["data"]["mode"] = "onboarding";
    ctx.AddCommand<TWebOSLaunchAppDirective>(NAlice::NVideoCommon::COMMAND_WEB_OS_LAUNCH_APP, onboardingPayload);
}

void AddWebOSLaunchAppCommandForVideoPlay(TContext& ctx, const NSc::TValue& payload) {
    if (!TryAddYoutubeWebOSLaunchAppCommand(ctx)) {
        NSc::TValue kpPayload;
        kpPayload["appId"] = KINOPOISK_APP_ID;
        auto& params = kpPayload["params"];
        params["pageId"] = "player";
        if (payload["item"]["type"].GetString() == "movie") {
            params["pageParam"] = payload["item"]["provider_item_id"].GetString();
            ctx.GetAnalyticsInfoBuilder().AddAction("video_play", "video play", TString::Join("Включается фильм \"", payload["item"]["name"].GetString(), "\""));
        } else {
            params["pageParam"] = payload["item"]["tv_show_item_id"].GetString();
            params["contentId"] = payload["item"]["provider_item_id"].GetString();
            ctx.GetAnalyticsInfoBuilder().AddAction("video_play", "video play", TString::Join("Включается сериал \"", payload["item"]["name"].GetString(), "\""));
        }

        ctx.AddCommand<TWebOSLaunchAppDirective>(NAlice::NVideoCommon::COMMAND_WEB_OS_LAUNCH_APP, kpPayload);
    }
}

void AddWebOSLaunchAppCommandForShowDescription(TContext& ctx, TVideoItemConstScheme item) {
    if (!TryAddYoutubeWebOSLaunchAppCommand(ctx)) {
        NSc::TValue kpPayload;
        kpPayload["appId"] = KINOPOISK_APP_ID;
        auto& params = kpPayload["params"];
        params["pageId"] = "film";
        params["pageParam"] = item.ProviderItemId();

        if (!ctx.IsAuthorizedUser()) {
            params["pageQuery"]["auth-deferred"].SetBool(true);
        }

        if (item.Type() == ToString(EItemType::Movie)) {
            ctx.GetAnalyticsInfoBuilder().AddAction("show_description", "show description", TString::Join("Открывается описание фильма \"", item.Name(), "\""));
        } else if (item.Type() == ToString(EItemType::TvShow)) {
            ctx.GetAnalyticsInfoBuilder().AddAction("show_description", "show description", TString::Join("Открывается описание сериала \"", item.Name(), "\""));
        }

        ctx.AddCommand<TWebOSLaunchAppDirective>(NAlice::NVideoCommon::COMMAND_WEB_OS_LAUNCH_APP, kpPayload);
    }
}

void AddWebOSLaunchAppCommandForShowGallery(TContext& ctx, TVideoGallery& gallery) {
    if (!TryAddYoutubeWebOSLaunchAppCommand(ctx)) {
        TryUpdateInfoIfNeeded(ctx, gallery);
        NSc::TValue galleryPayload;
        auto& itemsJson = galleryPayload["itemsJson"];
        TStringBuilder actionStr;
        actionStr << "Открывается галерея из " << gallery->Items().Size() << " фильмов/сериалов по запросу пользователя. Первые элементы списка:\n";
        for (ui32 i = 0; i < gallery->Items().Size(); i++) {
            const auto& videoItem = gallery->Items(i);
            NSc::TValue itemJson;
            TString horizontalPoster(videoItem->VhLicenses()->HorizontalPoster());
            if (horizontalPoster.Empty()) {
                horizontalPoster = videoItem->CoverUrl16X9();
            }

            if (horizontalPoster.StartsWith("//")) {
                horizontalPoster.prepend("https:");
            }
            itemJson["horizontalPoster"] = horizontalPoster;
            itemJson["kpRating"] = videoItem->Rating();
            auto& licenses = itemJson["monetizationModels"];
            if (!NSc::TValue::FromJson(videoItem->VhLicenses()->Est()).IsNull()) {
                licenses["est"] = NSc::TValue::FromJson(videoItem->VhLicenses()->Est());
            }
            if (!NSc::TValue::FromJson(videoItem->VhLicenses()->Tvod()).IsNull()) {
                licenses["tvod"] = NSc::TValue::FromJson(videoItem->VhLicenses()->Tvod());
            }
            if (!NSc::TValue::FromJson(videoItem->VhLicenses()->Svod()).IsNull()) {
                licenses["svod"] = NSc::TValue::FromJson(videoItem->VhLicenses()->Svod());
            }

            itemJson["title"] = videoItem->Name();
            auto& launchParams = itemJson["launchParams"];
            launchParams["appId"] = KINOPOISK_APP_ID;
            auto& params = launchParams["params"];
            params["pageId"] = "player";
            params["pageParam"] = videoItem->ProviderItemId();

            if (!ctx.IsAuthorizedUser()) {
                params["pageQuery"]["auth-deferred"].SetBool(true);
            }

            itemsJson.Push(std::move(itemJson));

            if (i < 3) {
                actionStr << i + 1 << ". " << videoItem->Name() << "\n";
            }
        }

        ctx.GetAnalyticsInfoBuilder().AddAction("show_gallery", "show gallery", actionStr);
        ctx.AddCommand<TWebOSShowGalleryDirective>(NAlice::NVideoCommon::COMMAND_WEB_OS_SHOW_GALLERY, galleryPayload);
    }
}

void AddWebOSLaunchAppCommandForShowSeasonGallery(TContext& ctx, const TVideoGallery& gallery) {
    if (!TryAddYoutubeWebOSLaunchAppCommand(ctx)) {
        NSc::TValue kpPayload;
        kpPayload["appId"] = KINOPOISK_APP_ID;
        auto& params = kpPayload["params"];
        params["pageId"] = "series";
        params["pageParam"] = gallery->TvShowItem()->ProviderItemId();
        ctx.GetAnalyticsInfoBuilder().AddAction("show_season_gallery", "show season gallery", TString::Join("Открывается список серий сериала \"", gallery->TvShowItem()->Name(), "\""));
        ctx.AddCommand<TWebOSLaunchAppDirective>(NAlice::NVideoCommon::COMMAND_WEB_OS_LAUNCH_APP, kpPayload);
    }
}

void AddWebOSLaunchAppCommandForPaymentScreen(TContext& ctx, const TCandidateToPlay& candidate) {
    if (!TryAddYoutubeWebOSLaunchAppCommand(ctx)) {
        const auto candidateCurr = candidate.Curr.Scheme();
        const auto candidateParent = candidate.Parent.Scheme();
        auto item = candidateCurr;
        if (candidateParent.Type() == ToString(EItemType::TvShow)) {
            item = candidateParent;
        }

        NSc::TValue kpPayload;
        kpPayload["appId"] = KINOPOISK_APP_ID;
        auto& params = kpPayload["params"];
        params["pageId"] = "film";  // Always send user to the description page, even if the payment is required
        params["pageParam"] = item.ProviderItemId();

        if (!ctx.IsAuthorizedUser()) {
            params["pageQuery"]["auth-deferred"].SetBool(true);
        }

        if (item.Type() == ToString(EItemType::Movie)) {
            ctx.GetAnalyticsInfoBuilder().AddAction("show_payment_screen", "show payment screen", TString::Join("Фильм платный, открывается описание фильма \"", item.Name(), "\""));
        } else if (item.Type() == ToString(EItemType::TvShow)) {
            ctx.GetAnalyticsInfoBuilder().AddAction("show_payment_screen", "show payment screen", TString::Join("Сериал платный, открывается описание сериала \"", item.Name(), "\""));
        }

        ctx.AddCommand<TWebOSLaunchAppDirective>(NAlice::NVideoCommon::COMMAND_WEB_OS_LAUNCH_APP, kpPayload);
    }
}

} // namespace NBASS::NVideo
