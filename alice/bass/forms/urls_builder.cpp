#include "urls_builder.h"

#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/uri/uri.h>

#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/subst.h>

namespace NBASS {

namespace {

TString PrintCgi(const TCgiParameters& cgi) {
    TString cgiStr = cgi.Print();
    SubstGlobal(cgiStr, "+", "%20"); // Some strange decoding of '+' symbol in mobile maps
    return cgiStr;
}
}

TString GenerateMapsUri(TContext& ctx, const TCgiParameters& cgi, bool needSimpleUrl) {
    // look for private key
    TString privateKeyStr = ctx.GlobalCtx().Secrets().NavigatorKey;
    if (!privateKeyStr) {
        LOG(ERR) << "Yandex.Navigator key was not found" << Endl;
    }
    return NAlice::GenerateMapsUri(ctx.ClientFeatures(), ctx.UserLocation(), "", cgi, privateKeyStr, ctx.ClientFeatures().SupportsIntentUrls(), needSimpleUrl);
}

TString GenerateNavigatorUri(const TContext& ctx, const TString& urlPrefix, const TCgiParameters& cgi, const TString& fallbackUrl) {
    // look for private key
    TString privateKeyStr = ctx.GlobalCtx().Secrets().NavigatorKey;
    if (!privateKeyStr) {
        LOG(ERR) << "Yandex.Navigator key was not found" << Endl;
    }
    return NAlice::GenerateNavigatorUri(ctx.ClientFeatures(), privateKeyStr, urlPrefix, cgi, fallbackUrl,
        ctx.ClientFeatures().SupportsIntentUrls(), ctx.ClientFeatures().SupportsNavigator());
}

TString GenerateTaxiUri(TContext& ctx, const NSc::TValue& from, const NSc::TValue& to, bool useFallback) {
    return NAlice::GenerateTaxiUri(ctx.ClientFeatures(), from, to, useFallback);
}

TString GenerateApplicationUri(const TContext& ctx, TStringBuf app, TStringBuf fallbackUrl) {
    return NAlice::GenerateApplicationUri(ctx.ClientFeatures(), app, fallbackUrl);
}

TString GenerateAuthorizationUri(const TContext& ctx) {
    return NAlice::GenerateAuthorizationUri(ctx.ClientFeatures().SupportsOpenYandexAuth());
}

TString GenerateMusicVerticalUri(const TContext& ctx) {
    return NAlice::GenerateMusicVerticalUri(ctx.ClientFeatures());
}

TString GeneratePhoneUri(const TClientInfo& clientInfo, const TString& phone, bool normalize, bool addPrefix) {
    return GeneratePhoneUri(clientInfo, TStringBuf(phone), normalize, addPrefix);
}

TString GeneratePhoneUri(const TClientInfo& clientInfo, TStringBuf phone, bool normalize, bool addPrefix) {
    return NAlice::GeneratePhoneUri(clientInfo, phone, normalize, addPrefix);
}

TString GenerateSearchUri(TContext* ctx, TStringBuf query, TCgiParameters cgi) {
    return NAlice::GenerateSearchUri(ctx->ClientFeatures(), ctx->UserLocation(), ctx->ContentRestrictionLevel(), query, ctx->ClientFeatures().SupportsOpenLinkSearchViewport(), cgi);
}

TString GenerateSearchAdsUri(TContext* ctx, TStringBuf query, TCgiParameters cgi) {
    return NAlice::GenerateSearchAdsUri(ctx->ClientFeatures(), ctx->UserLocation(), ctx->ContentRestrictionLevel(), query, cgi);
}

TString GenerateNewsUri(const TClientInfo& clientInfo, TStringBuf dirtyUrl) {
    return NAlice::GenerateNewsUri(clientInfo, dirtyUrl);
}

TString GenerateWeatherUri(const TClientInfo& clientInfo, TStringBuf origUri, ui16 mday) {
    return NAlice::GenerateWeatherUri(clientInfo, origUri, mday);
}

TString GenerateMapsTrafficUri(TContext& ctx, NGeobase::TId regionId) {
    // look for private key
    TString privateKeyStr = ctx.GlobalCtx().Secrets().NavigatorKey;
    if (!privateKeyStr) {
        LOG(ERR) << "Yandex.Navigator key was not found" << Endl;
    }
    return NAlice::GenerateMapsTrafficUri(ctx.ClientFeatures(), ctx.UserLocation(), privateKeyStr, ctx.GlobalCtx().GeobaseLookup(),
        regionId, ctx.ClientFeatures().SupportsIntentUrls(), ctx.ClientFeatures().SupportsNavigator());
}

void InitAppleSchemes() {
    NAlice::InitAppleSchemes();
}

TString GenerateVideoSerpUrl(TContext* context, TStringBuf query, TCgiParameters& cgi) {
    TStringBuilder url;
    if (context->MetaClientInfo().IsSearchApp()) {
        url << "viewport://?";
        cgi.InsertUnescaped(TStringBuf("viewport_id"), TStringBuf("video"));
        cgi.InsertUnescaped(TStringBuf("noreask"), TStringBuf("1"));
    } else if (context->MetaClientInfo().IsTouch()) {
        url << "https://yandex." << context->UserTld() << "/video/touch/search?";
    } else {
        url << "https://yandex." << context->UserTld() << "/video/search/?";
    }
    cgi.InsertUnescaped(TStringBuf("text"), query);
    NGeobase::TId lr = context->UserRegion();
    if (NAlice::IsValidId(lr)) {
        cgi.InsertUnescaped(TStringBuf("lr"), ToString(lr));
    } else {
        // если не удалось определить lr, то не шлём его совсем
        // в этом случае это не страшно, т.к. это ссылка для браузера/web view,
        // и lr там определится на общих основаниях
    }
    if (context->GetContentRestrictionLevel() == EContentRestrictionLevel::Children)
        cgi.InsertUnescaped(TStringBuf("fyandex"), TStringBuf("1"));
    if (context->Meta().HasLang())
        cgi.InsertUnescaped(TStringBuf("l10n"), context->Meta().Lang());
    cgi.InsertUnescaped(TStringBuf("reqid"), context->ReqId());
    url << PrintCgi(cgi);
    return url;
}

TString GenerateSimilarImagesSearchUrl(TContext& ctx, TStringBuf cbirId,
                                       TStringBuf aliceSource, TStringBuf cbirPage, TStringBuf report,
                                       bool needNextPage, bool disablePtr, int cropId) {
    return NAlice::GenerateSimilarImagesSearchUrl(ctx.ClientFeatures(), cbirId, aliceSource, cbirPage, report, needNextPage, disablePtr, cropId);
}

TString GenerateSimilarsGalleryLink(TContext& ctx, TStringBuf cbirId, TStringBuf imgUrl) {
    return NAlice::GenerateSimilarsGalleryLink(ctx.ClientFeatures(), cbirId, imgUrl);
}

TString GenerateMarketDealsLink(TStringBuf cbirId, TStringBuf uuid) {
    return NAlice::GenerateMarketDealsLink(cbirId, uuid);
}

TString GenerateTranslateUri(TContext& ctx, TStringBuf text, TStringBuf dir) {
    return NAlice::GenerateTranslateUri(ctx.ClientFeatures(), ctx.UserTld(), text, dir);
}

TString GenerateMessengerUri(TStringBuf phone, TStringBuf client, TStringBuf text, TStringBuf fallback) {
    return NAlice::GenerateMessengerUri(phone, client, text, fallback);
}

TString AddUtmReferrer(const TClientInfo& clientInfo, TStringBuf url) {
    return NAlice::AddUtmReferrer(clientInfo, url);
}

} // namespace NBASS
