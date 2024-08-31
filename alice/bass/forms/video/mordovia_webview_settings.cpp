#include "mordovia_webview_settings.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/video/utils.h>

#include <alice/library/video_common/mordovia_webview_defs.h>

using namespace NBASS::NVideo;

namespace NAlice::NVideoCommon {

TString GetWebViewVideoHost(const NBASS::TContext& ctx) {
    return GetStringSettingsFromExp(ctx, FLAG_WEBVIEW_VIDEO_HOST, DEFAULT_WEBVIEW_VIDEO_HOST);
}

TString GetWebViewVideoGalleryPath(const NBASS::TContext& ctx) {
    return GetStringSettingsFromExp(ctx, FLAG_WEBVIEW_VIDEO_GALLERY_PATH, DEFAULT_VIDEO_GALLERY_PATH);
}

TString GetWebViewVideoGallerySplash(const NBASS::TContext& ctx) {
    return GetStringSettingsFromExp(ctx, FLAG_WEBVIEW_VIDEO_GALLERY_SPLASH, DEFAULT_VIDEO_GALLERY_SPLASH);
}

TString GetWebViewFilmsGalleryPath(const NBASS::TContext& ctx) {
    return GetStringSettingsFromExp(ctx, FLAG_WEBVIEW_FILMS_GALLERY_PATH, DEFAULT_FILMS_GALLERY_PATH);
}

TString GetWebViewFilmsGallerySplash(const NBASS::TContext& ctx) {
    return GetStringSettingsFromExp(ctx, FLAG_WEBVIEW_FILMS_GALLERY_SPLASH, DEFAULT_FILMS_GALLERY_SPLASH);
}

TString GetWebViewVideoSingleCardPath(const NBASS::TContext& ctx) {
    return GetStringSettingsFromExp(ctx, FLAG_WEBVIEW_VIDEO_ENTITY_PATH, DEFAULT_VIDEO_SINGLE_CARD_PATH);
}

TString GetWebViewVideoSingleCardSplash(const NBASS::TContext& ctx) {
    return GetStringSettingsFromExp(ctx, FLAG_WEBVIEW_VIDEO_ENTITY_SPLASH, ToString(DEFAULT_VIDEO_SINGLE_CARD_SPLASH));
}

TString GetWebViewVideoDescriptionCardPath(const NBASS::TContext& ctx) {
    return GetStringSettingsFromExp(ctx, FLAG_WEBVIEW_VIDEO_DESCRIPTION_CARD_PATH, ToString(DEFAULT_VIDEO_DESCRIPTION_CARD_PATH));
}

TString GetWebViewVideoDescriptionCardSplash(const NBASS::TContext& ctx) {
    return GetStringSettingsFromExp(ctx, FLAG_WEBVIEW_VIDEO_DESCRIPTION_CARD_SPLASH, ToString(DEFAULT_VIDEO_DESCRIPTION_CARD_SPLASH));
}

TString GetWebViewSeasonsGalleryPath(const NBASS::TContext& ctx) {
    return GetStringSettingsFromExp(ctx, FLAG_WEBVIEW_VIDEO_ENTITY_SEASONS_PATH, ToString(DEFAULT_VIDEO_SEASONS_GALLERY_PATH));
}

TString GetWebViewSeasonsGallerySplash(const NBASS::TContext& ctx) {
    return GetStringSettingsFromExp(ctx, FLAG_WEBVIEW_VIDEO_ENTITY_SEASONS_SPLASH, ToString(DEFAULT_VIDEO_SEASONS_GALLERY_SPLASH));
}

TString GetWebViewChannelsPath(const NBASS::TContext& ctx) {
    return GetStringSettingsFromExp(ctx, FLAG_WEBVIEW_CHANNELS_PATH, ToString(DEFAULT_CHANNELS_PATH));
}

TString GetWebViewChannelsSplash(const NBASS::TContext& ctx) {
    return GetStringSettingsFromExp(ctx, FLAG_WEBVIEW_CHANNELS_SPLASH, ToString(DEFAULT_CHANNELS_SPLASH));
}

TString GetWebViewPromoSplash(const NBASS::TContext& ctx) {
    return GetStringSettingsFromExp(ctx, FLAG_WEBVIEW_VIDEO_PROMO_SPLASH, ToString(DEFAULT_PROMO_NY_SPLASH));
}

TString BuildWebViewVideoUrl(TStringBuf host, TStringBuf path, const TCgiParameters& params) {
    return TStringBuilder() << host << path << (path.Contains('?') ? "&" : "?") << params.Print();
}

TString BuildWebViewVideoPath(TStringBuf path, const TCgiParameters& params) {
    return BuildWebViewVideoUrl(""/* host */, path, params);
}

} // namespace NAlice::NVideoCommon
