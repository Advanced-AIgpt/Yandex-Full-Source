#pragma once

#include <alice/library/restriction_level/protos/content_settings.pb.h>

#include <alice/library/client/client_info.h>
#include <alice/library/client/client_features.h>
#include <alice/library/geo/user_location.h>

#include <library/cpp/cgiparam/cgiparam.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice {

TString PrintCgi(const TCgiParameters& cgi);

/** Generate url, that will be opened in Yandex.Maps mobile app (with fallback into viewport_id=maps)
 * @param[in] ctx - context object
 * @param[in] cgi - cgi parameters for map url
 * @param[in] needSimpleUrl - whether we need to generate simple http url for web maps
 * @return string with url
 */
TString GenerateMapsUri(const TClientInfo& clientInfo, const TUserLocation& location, const TStringBuf path, const TCgiParameters& cgi,
        const TStringBuf privateNavigatorKey, bool supportsIntentUrls, bool needSimpleUrl = false, const TStringBuf fallbackUrl = "");

/** Generate url, that will be opened in Yandex.Navigator mobile app.
 * If Yandex.Navigator is not installed, then fallbackUrl will be opened.
 * @param[in] ctx - context object
 * @param[in] urlPrefix - url prefix, identifying what to do (e.g. "build_route_on_map?")
 * @param[in] cgi - cgi parameters for url
 * @return string with url
 * @see https://wiki.yandex-team.ru/users/vralex/navigator-intents/
 */
TString GenerateNavigatorUri(const TClientInfo& clientInfo, const TString& privateNavigatorKey, const TString& urlPrefix,
                             const TCgiParameters& cgi, const TString& fallbackUrl, bool supportsIntentUrls,
                             bool supportsNavigator);

/**
 * Generate url, that will be opened in the application <code>app</code> (if it installed),
 * otherwise the <code>fallbackUrl</code> will be opened in the default web browser.
 *
 * @param [in] ctx current context
 * @param [in] app package name for GPlay and package id for iTunes
 * @param [in] fallbackUrl fallback url to open in a web browser if the application <code>app</code> not installed
 * @return uri
 */
TString GenerateApplicationUri(const TClientFeatures& clientFeatures, TStringBuf app, TStringBuf fallbackUrl);
TString GenerateApplicationUri(const bool supportsIntentUrls, const TClientInfo& clientInfo, TStringBuf app, TStringBuf fallbackUrl);

/**
 * Generate url for user authorisation. Might return an empty string!
 * @param [in] supportsOpenYandexAuth does client support yandex-auth:// links
 * @return uri or ""
 */
TString GenerateAuthorizationUri(const bool supportsOpenYandexAuth);

/**
 * Generate url to music vertical
 * @param [in] ctx current context
 * @return uri
 */
TString GenerateMusicVerticalUri(const TClientFeatures& clientFeatures);
TString GenerateMusicVerticalUri(const bool hasMusicSdkClient, const TClientInfo& clientInfo);

enum class EMusicUriType { Music, Radio, RadioPersonal };

/**
 * Generate url, that will be opened in the YaMusic/YaRadio app (if it installed),
 * otherwise the <code>fallbackUrl</code> will be opened in the default web browser.
 *
 * @param [in] ctx — current context
 * @param [in] isRadio — flag for generating radio link
 * @param [in] path — path to object to open
 * @param [in] cgi — additional cgi parameters (autoplay, etc)
 * @return uri
 */
TString GenerateMusicAppUri(const TClientFeatures& clientFeatures, EMusicUriType uriType, TStringBuf path, TCgiParameters cgi = TCgiParameters());
TString GenerateMusicAppUri(const bool supportsIntentUrls, const TClientInfo& clientInfo, EMusicUriType uriType, TStringBuf path, TCgiParameters cgi = TCgiParameters());

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
TString GenerateSearchUri(const TClientInfo& clientInfo, const TUserLocation& location, EContentSettings restrictionLevel,
                          TStringBuf query, bool canOpenLinkSearchViewport, TCgiParameters cgi = {}, bool forceViewport = false);

/**
 * Generate search/ads URI.
 */
TString GenerateSearchAdsUri(const TClientInfo& clientInfo, const TUserLocation& location, EContentSettings restrictionLevel,
                             TStringBuf query, TCgiParameters cgi = {});

/**
 * Generate geo object open URI.
 */
TString GenerateObjectUri(const TClientInfo& clientInfo, const TUserLocation& location, EContentSettings restrictionLevel,
                          const TString& objectId, const TString& objectName, const TString& seoName, bool canOpenLinkSearchViewport);

/**
 * Generate address geolocation object open URI.
 */
TString GenerateObjectCatalogUri(const TUserLocation& location, const TString& objectId,
                                 bool rewiewsIntent = false, bool photoIntent = false);

/**
 * Generate url for news
 */
TString GenerateNewsUri(const TClientInfo& clientInfo, TStringBuf dirtyUrl);

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
TString GenerateMapsTrafficUri(const TClientInfo& clientInfo, const TUserLocation& location,
                               const TString& privateNavigatorKey, const NGeobase::TLookup& geobase, NGeobase::TId regionId,
                               bool supportsIntentUrls, bool supportsNavigator);

/** Generate url for taxi
 * @param[in] ctx is a request context
 * @param[in] from is a start geo
 * @param[in] to is a finish geo
 * @param[in] useFallback indicates whether to open the url of the site in a taxi,
 * if the user has no applications for a taxi. If false, it will open store
 */
TString GenerateTaxiUri(const TClientFeatures& clientFeatures, const NSc::TValue& from, const NSc::TValue& to, bool useFallback = true);

/**
 * Generate url for similar images search from computer vision
 * @param[in] ctx is a request context
 * @param[in] cbirId is a special image id from computer vision
 * @param[in] aliceSource is a text value to identify origins of similar images search urls
 * @param[in] report is upper type for request processing
 * @param[in] disablePtr disables reload page by swype
 * @return url
 */
TString GenerateSimilarImagesSearchUrl(const TClientInfo& clientInfo, TStringBuf cbirId,
                                       TStringBuf aliceSource, TStringBuf cbirPage, TStringBuf report,
                                       bool needNextPage, bool disablePtr, int cropId = -1);

/**
 * Generate url for links from similars gallery
 * @param[in] ctx is a request context
 * @param[in] cbirId is a special image id from computer vision
 * @param[in] imgUrl is an image url provided by computer vision
 * @return url
 */
TString GenerateSimilarsGalleryLink(const TClientInfo& clientInfo, TStringBuf cbirId, TStringBuf imgUrl);

/**
 * Generate url for links to all Market deals in Computer Vision Market mode
 * @param[in] cbirId is a special image id from computer vision
 * @param[iin] uuid is an user identifier
 * @return url
 */
TString GenerateMarketDealsLink(TStringBuf cbirId, TStringBuf uuid);

/**
 * Generate url for Yandex.Translate scenario
 * @param[in] clientFeatures client features
 * @param[tld] Top-Level-Domain
 * @param[in] text is a text to translate
 * @param[in] dir is a language direction for translation
 * @return url
 */
TString GenerateTranslateUri(const TClientFeatures& clientFeatures, TStringBuf tld, TStringBuf text, TStringBuf dir);
TString GenerateTranslateUri(const bool supportsIntentUrls, const TClientInfo& clientInfo, TStringBuf tld, TStringBuf text, TStringBuf dir);

/**
 * Generate Products Search URI.
 */
TString GenerateProductsSearchUri(const TStringBuf query, const bool canOpenLinkSearchViewport);

struct TClientActionUrl {
    enum class EType {
        Empty      /* "" its just a stub for default construct and default value */,
        ShowTimers /* "show_timers" */,
        ShowAlarms /* "show_alarms" */,
        OpenDialog /* "open_dialog" */,
        Text       /* "type" */,
    };

    explicit TClientActionUrl(EType clientAction);
    TClientActionUrl(EType clientAction, NSc::TValue payload);

    TString ToString() const;

    static TString ToString(const TVector<TClientActionUrl>& actions);
    static TString OpenDialogByText(TStringBuf text, const TMaybe<NSc::TValue>& payload = Nothing());
    static TString OpenDialogById(TStringBuf dialogId, const TMaybe<NSc::TValue>& payload = Nothing());

    EType Type;
    TMaybe<NSc::TValue> Payload = Nothing();
};

TString AddUtmReferrer(const TClientInfo& client, TStringBuf url);

TString GenerateMessengerUri(TStringBuf phone, TStringBuf client, TStringBuf text, TStringBuf fallback);

}  // namespace NAlice
