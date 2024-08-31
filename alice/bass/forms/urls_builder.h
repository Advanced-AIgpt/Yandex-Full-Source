#pragma once

#include <alice/library/url_builder/url_builder.h>

#include <alice/bass/forms/context/context.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NBASS {

/** Generate url, that will be opened in Yandex.Maps mobile app (with fallback into viewport_id=maps)
 * @param[in] ctx - context object
 * @param[in] cgi - cgi parameters for map url
 * @param[in] needSimpleUrl - whether we need to generate simple http url for web maps
 * @return string with url
 */
TString GenerateMapsUri(TContext& ctx, const TCgiParameters& cgi, bool needSimpleUrl = false);

/** Generate url, that will be opened in Yandex.Navigator mobile app.
 * If Yandex.Navigator is not installed, then fallbackUrl will be opened.
 * @param[in] ctx - context object
 * @param[in] urlPrefix - url prefix, identifying what to do (e.g. "build_route_on_map?")
 * @param[in] cgi - cgi parameters for url
 * @return string with url
 * @see https://wiki.yandex-team.ru/users/vralex/navigator-intents/
 */
TString GenerateNavigatorUri(const TContext& ctx, const TString& urlPrefix, const TCgiParameters& cgi,
                             const TString& fallbackUrl);

/**
 * Generate url, that will be opened in the application <code>app</code> (if it installed),
 * otherwise the <code>fallbackUrl</code> will be opened in the default web browser.
 *
 * @param [in] ctx current context
 * @param [in] app package name for GPlay and package id for iTunes
 * @param [in] fallbackUrl fallback url to open in a web browser if the application <code>app</code> not installed
 * @return uri
 */
TString GenerateApplicationUri(const TContext& ctx, TStringBuf app, TStringBuf fallbackUrl);

/**
 * Generate url for user authorization. Might return an empty string!
 * @param [in] ctx current context
 * @return uri or ""
 */
TString GenerateAuthorizationUri(const TContext& ctx);

/**
 * Generate url to music vertical
 * @param [in] ctx current context
 * @return uri
 */
TString GenerateMusicVerticalUri(const TContext& ctx);

using EMusicUriType = NAlice::EMusicUriType;

/**
 * Generate device specific phone uri.
 *
 * @param [in] clientInfo client device information
 * @param [in] phone 'durty' phone number
 * @return uri
 */
TString GeneratePhoneUri(const TClientInfo& clientInfo, const TString& phone, bool normalize = true, bool addPrefix = true);
TString GeneratePhoneUri(const TClientInfo& clientInfo, TStringBuf phone, bool normalize = true, bool addPrefix = true);

/**
 * Generate search URI.
 */
TString GenerateSearchUri(TContext* ctx, TStringBuf query, TCgiParameters cgi = TCgiParameters());

/**
 * Generate search/ads URI.
 */
TString GenerateSearchAdsUri(TContext* ctx, TStringBuf query, TCgiParameters cgi = TCgiParameters());

/** Generate url for news
 */
TString GenerateNewsUri(const TClientInfo& ci, TStringBuf dirtyUrl);

/**
 * Init data for iOS intent links
 */
void InitAppleSchemes();

/** Generate url for weather.
 * @param[in] clientInfo is client information
 * @param[in] origUri is an original uri from weather
 * @param[in] mday is a day of the months 1..3X
 */
TString GenerateWeatherUri(const TClientInfo& clientInfo, TStringBuf origUri, ui16 mday);

/** Generate url for traffice
 * @param[in] ctx is a request context
 * @param[in] regionId is a city with traffic
 */
TString GenerateMapsTrafficUri(TContext& ctx, NGeobase::TId regionId);

/** Generate url for taxi
 * @param[in] ctx is a request context
 * @param[in] from is a start geo
 * @param[in] to is a finish geo
 * @param[in] useFallback indicates whether to open the url of the site in a taxi,
 * if the user has no applications for a taxi. If false, it will open store
 */
TString GenerateTaxiUri(TContext& ctx, const NSc::TValue& from, const NSc::TValue& to, bool useFallback = true);
/**
 * Generate url for Video vertical.
 * @param [in] context request context
 * @param [in] query search query
 * @param [in] cgi additional cgi parameters
 * @return url
 */
TString GenerateVideoSerpUrl(TContext* context, TStringBuf query, TCgiParameters& cgi);

/**
 * Generate url for similar images search from computer vision
 * @param[in] ctx is a request context
 * @param[in] cbirId is a special image id from computer vision
 * @param[in] aliceSource is a text value to identify origins of similar images search urls
 * @param[in] report is upper type for request processing
 * @param[in] disablePtr disables reload page by swype
 * @return url
 */
TString GenerateSimilarImagesSearchUrl(TContext& ctx, TStringBuf cbirId,
                                       TStringBuf aliceSource, TStringBuf cbirPage, TStringBuf report,
                                       bool needNextPage, bool disablePtr, int cropId = -1);

/**
 * Generate url for links from similars gallery
 * @param[in] ctx is a request context
 * @param[in] cbirId is a special image id from computer vision
 * @param[in] imgUrl is an image url provided by computer vision
 * @return url
 */
TString GenerateSimilarsGalleryLink(TContext& ctx, TStringBuf cbirId, TStringBuf imgUrl);

/**
 * Generate url for links to all Market deals in Computer Vision Market mode
 * @param[in] cbirId is a special image id from computer vision
 * @param[in] uuid is an user identifier
 * @return url
 */
TString GenerateMarketDealsLink(TStringBuf cbirId, TStringBuf uuid);

/**
 * Generate url for Yandex.Translate scenario
 * @param[in] ctx is a request context
 * @param[in] text is a text to translate
 * @param[in] dir is a language direction for translation
 * @return url
 */
TString GenerateTranslateUri(TContext& ctx, TStringBuf text, TStringBuf dir);

using TClientActionUrl = NAlice::TClientActionUrl;

TString AddUtmReferrer(const TClientInfo& client, TStringBuf url);

TString GenerateMessengerUri(TStringBuf phone, TStringBuf client, TStringBuf text, TStringBuf fallback);

}  // namespace NBASS
