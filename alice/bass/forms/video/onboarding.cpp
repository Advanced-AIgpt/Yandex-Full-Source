#include "onboarding.h"

#include "alice/bass/forms/video/defs.h"
#include "utils.h"

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/music/providers.h>
#include <alice/bass/forms/player/player.h>
#include <alice/bass/forms/radio.h>
#include <alice/bass/forms/tv/defs.h>
#include <alice/bass/forms/external_skill_recommendation/skill_recommendation.h>

#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/push_notification/handlers/onboarding_push.h>

#include <dict/dictutil/str.h>

namespace NBASS {
namespace NVideo {

namespace {

TResultValue DoStartOnboarding(TStringBuf mode, TContext& ctx) {
    TSlot* slot = ctx.GetOrCreateSlot(TStringBuf("mode"), TStringBuf("string"));

    TStringBuilder fullMode;
    fullMode << TStringBuf("quasar_") << mode;

    ReplaceAll(fullMode, '/', '_');

    slot->Value.SetString(fullMode);

    return TResultValue();
}

} // namespace anonymous

TResultValue StartOnboarding(TContext& ctx) {
    TSlot* number = ctx.GetOrCreateSlot(TStringBuf("set_number"), TStringBuf("num"));
    int startNumber = 0;
    if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_VIDEO_ONBOARDING_RANDOMIZE)) {
        //Only VINS knows how many "cards" each screen has, so it always uses % to cut the rest
        startNumber = ctx.GetRng().RandomInteger(10000);
    }
    number->Value.SetIntNumber(startNumber);

    TStringBuf textNum;
    ctx.OnEachExpFlag([&textNum](auto expFlag) {
        if (expFlag.StartsWith(EXPERIMENTAL_FLAG_VIDEO_ONBOARDING_SEND_PUSH_PREFIX)) {
            textNum = expFlag.SubStr(EXPERIMENTAL_FLAG_VIDEO_ONBOARDING_SEND_PUSH_PREFIX.size());
        }
    });
    if (!textNum.empty()) {
        NSc::TValue serviceData;
        serviceData["text_num"] = textNum;
        ctx.SendPushRequest("onboarding", NPushNotification::ONBOARDING_EVENT, Nothing(), serviceData);
    }

    const auto& state = ctx.Meta().DeviceState();
    if (!state.IsTvPluggedIn()) {
        if (!NPlayer::IsMusicPaused(ctx)) {
            return DoStartOnboarding(TStringBuf("music_playing"), ctx);
        } else if (!NPlayer::IsRadioPaused(ctx)) {
            return DoStartOnboarding(TStringBuf("radio_player"), ctx);
        }
        return DoStartOnboarding(ToString(EScreenId::Main), ctx);
    }

    TMaybe<EScreenId> screen = GetCurrentScreen(ctx);
    if (!screen) {
        return DoStartOnboarding(ToString(EScreenId::Main), ctx);
    }

    if (screen == EScreenId::VideoPlayer) {
        // we have no "tv_player" or "tv_player_personal" screen, but should use special onboarding text,
        // when video_player is playing tv-stream content
        TSchemeHolder<NVideo::TVideoItemScheme> holder = NVideo::GetCurrentVideoItem(ctx);
        NVideo::TVideoItemScheme currentItem = holder.Scheme();
        if (currentItem.Type() == ToString(NVideo::EItemType::TvStream)) {
            TStringBuf playerType = currentItem.TvStreamInfo().IsPersonal() ? TStringBuf("tv_player_personal") : TStringBuf("tv_player");
            return DoStartOnboarding(playerType, ctx);
        }
    }

    switch (*screen) {
    case EScreenId::Main:
        [[fallthrough]];
    case EScreenId::TvMain:
        [[fallthrough]];
    case EScreenId::MordoviaMain:
        if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_DJ_SERVICE_FOR_ONBOARDING_SMARTSPEAKER_MAIN)) {
            if (NExternalSkill::TSkillRecommendationInitializer::SetAsResponse(ctx, NExternalSkill::EServiceRequestCard::Onboarding)) {
                return ctx.RunResponseFormHandler();
            }
        }
    case EScreenId::Gallery:
        [[fallthrough]];
    case EScreenId::WebViewFilmsSearchGallery:
        [[fallthrough]];
    case EScreenId::WebViewVideoSearchGallery:
        [[fallthrough]];
    case EScreenId::TvExpandedCollection:
        [[fallthrough]];
    case EScreenId::SearchResults:
        [[fallthrough]];
    case EScreenId::SeasonGallery:
        [[fallthrough]];
    case EScreenId::TvGallery:
        [[fallthrough]];
    case EScreenId::WebViewChannels:
        [[fallthrough]];
    case EScreenId::Description:
        [[fallthrough]];
    case EScreenId::ContentDetails:
        [[fallthrough]];
    case EScreenId::WebViewVideoEntity:
        [[fallthrough]];
    case EScreenId::WebviewVideoEntityWithCarousel:
        [[fallthrough]];
    case EScreenId::WebviewVideoEntityDescription:
        [[fallthrough]];
    case EScreenId::WebviewVideoEntitySeasons:
        [[fallthrough]];
    case EScreenId::WebviewVideoEntityRelated:
        [[fallthrough]];
    case EScreenId::Payment:
        [[fallthrough]];
    case EScreenId::VideoPlayer:
        [[fallthrough]];
    case EScreenId::RadioPlayer:
        [[fallthrough]];
    case EScreenId::MusicPlayer:
        [[fallthrough]];
    case EScreenId::Bluetooth:
        return DoStartOnboarding(ToString(*screen), ctx);
    }
    Y_UNREACHABLE();
}

TResultValue ContinueOnboarding(TContext& ctx) {
    TSlot* number = ctx.GetOrCreateSlot(TStringBuf("set_number"), TStringBuf("num"));
    ++number->Value.GetIntNumberMutable(); // VINS will cut remainder
    return TResultValue();
}

} // namespace NVideo
} // namespace NBASS
