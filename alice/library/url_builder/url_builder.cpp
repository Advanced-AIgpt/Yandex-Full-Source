#include "url_builder.h"

#include <contrib/libs/openssl/include/openssl/sha.h>
#include <contrib/libs/openssl/include/openssl/rsa.h>
#include <contrib/libs/openssl/include/openssl/obj_mac.h>
#include <contrib/libs/openssl/include/openssl/pem.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/uri/uri.h>

#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/subst.h>
#include <util/string/builder.h>
#include <util/string/cast.h>

namespace NAlice {

namespace {

constexpr TStringBuf SEARCH_APP_AUTH_URI = "yandex-auth://?theme=light";
constexpr TStringBuf YNAVIGATOR_CLIENT_ID = "005";
constexpr TStringBuf NAVIGATOR_URL_PREFIX = "yandexnavi";
constexpr TStringBuf MAPS_URL_PREFIX = "yandexmaps";

NSc::TValue AppleSchemes;

struct TBIODeleter {
    static void Destroy(BIO* bio) {
        if (bio) {
            BIO_free(bio);
        }
    }
};

struct TRSADeleter {
    static void Destroy(RSA* rsa) {
        if (rsa) {
            RSA_free(rsa);
        }
    }
};

using TBIOHolder = THolder<BIO, TBIODeleter>;
using TRSAHolder = THolder<RSA, TRSADeleter>;
using TAddActionCallback = std::function<void(TClientActionUrl::EType type, const TMaybe<NSc::TValue>& payload)>;

template <typename TCallback>
TString CreateClientActionUrl(const TCallback& cb) {
    TStringBuilder builder;
    builder << TStringBuf("dialog-action://");

    NSc::TValue directives;
    auto cbAddDirective = [&directives](TClientActionUrl::EType type, const TMaybe<NSc::TValue>& payload) {
        TString typeStr{ToString(type)};
        if (!typeStr) {
            return;
        }
        NSc::TValue& item = directives.Push();
        item["type"].SetString(TStringBuf("client_action"));
        item["name"].SetString(typeStr);
        if (payload) {
            item["payload"] = *payload;
        }
    };
    cb(cbAddDirective);
    if (!directives.IsNull()) {
        TCgiParameters cgi;
        cgi.InsertUnescaped("directives", directives.ToJson());
        builder << '?' << PrintCgi(cgi);
    }

    return builder;
}

TString CreateClientActionDialog(TStringBuf key, TStringBuf value, TClientActionUrl::EType type, const TMaybe<NSc::TValue>& maybePayload) {
    return CreateClientActionUrl(
        [key, value, type, &maybePayload](TAddActionCallback addAction) {
            NSc::TValue payload;
            if (maybePayload) {
                payload = maybePayload->Clone();
            }
            payload[key].SetString(value);
            addAction(type, payload);
        }
    );
}

TString PatchNewsUrl(TStringBuf url) {
    NUri::TUri uri;
    if (uri.Parse(url, NUri::TUri::TFeature::FeaturesRecommended) == NUri::TUri::ParsedOK) {
        if (uri.GetHost() == TStringBuf("m.news.yandex.ru")) {
            NUri::TUriUpdate(uri).Set(NUri::TField::FieldHost, "news.yandex.ru");
        }
        const TStringBuf& query = uri.GetField(NUri::TField::FieldQuery);
        TCgiParameters cgiParams;
        cgiParams.Scan(query);
        if (cgiParams.EraseAll("appsearch_header")) {
            TString cgiParamsString = cgiParams.Print();
            NUri::TUriUpdate(uri).Set(NUri::TField::FieldQuery, cgiParamsString);
        }
        return uri.PrintS();
    }

    return ToString(url);
}

void SignGeoUri(const TStringBuf privateNavigatorKey, const TStringBuf uriPrefix, TStringBuilder& geoUri) {
    if (!privateNavigatorKey) {
        return;
    }

    // get RSA private key
    TBIOHolder bio = TBIOHolder(BIO_new_mem_buf(privateNavigatorKey.data(), static_cast<int>(privateNavigatorKey.size())));
    if (!bio) {
        return;
    }

    TRSAHolder privateKey = TRSAHolder(PEM_read_bio_RSAPrivateKey(bio.Get(), nullptr, nullptr, nullptr));
    if (!privateKey) {
        return;
    }

    // make uri to sign - add scheme and client param
    TStringBuilder uriToSign;
    uriToSign << uriPrefix << "://"
              << geoUri
              << TStringBuf("&client=") << CGIEscapeRet(YNAVIGATOR_CLIENT_ID);

    // get SHA-256 hash from uri
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(uriToSign.data()), uriToSign.size(), digest);

    // sign hash with private key
    const unsigned int rsaSize = RSA_size(privateKey.Get());
    unsigned char signature[rsaSize];
    unsigned int outlen = 0;

    int signSuccessfull = RSA_sign(NID_sha256,
                                   digest,
                                   SHA256_DIGEST_LENGTH,
                                   signature,
                                   &outlen,
                                   privateKey.Get());

    if (!signSuccessfull) {
        return;
    }

    // ecsape base64-encoded signature
    TStringBuf signatureStr(reinterpret_cast<const char*>(signature), outlen);
    TString signatureEscaped = Base64Encode(signatureStr);
    Quote(signatureEscaped, "");

    // finally add signature to geo uri
    geoUri << TStringBuf("&client=") << CGIEscapeRet(YNAVIGATOR_CLIENT_ID)
                 << TStringBuf("&signature=") << signatureEscaped;
}

void AddTaxiLoggingParams(TCgiParameters* taxiParams) {
    taxiParams->InsertUnescaped(TStringBuf("utm_source"), TStringBuf("integration"));
    taxiParams->InsertUnescaped(TStringBuf("utm_medium"), TStringBuf("alice"));
    taxiParams->InsertUnescaped(TStringBuf("ref"), TStringBuf("2423271"));
}

TString BuildIOsIntent(const TClientInfo& clientInfo, TString intentUrl, TString fallbackUrl) {
    TStringBuilder intent;
    Quote(intentUrl, "");
    intent << (clientInfo.IsNavigator() ? TStringBuf("vins://open_url_with_fallback?url=") : TStringBuf("intent:?url="))
           << intentUrl;

    if (!fallbackUrl.empty()) {
        Quote(fallbackUrl, "");
        intent << TStringBuf("&fallback_url=") << fallbackUrl;
    }

    return intent;
}

TString BuildAndroidIntent(const TClientInfo& clientInfo, TStringBuf scheme, TStringBuf package, TString intentUrl, TString fallbackUrl) {
    TStringBuilder intent;
    intent << TStringBuf("intent://") << intentUrl
           << TStringBuf("#Intent");
    if (!scheme.empty()) {
        intent << TStringBuf(";scheme=") << scheme;
    }
    if (!package.empty()) {
        intent << TStringBuf(";package=") << package;
    }

    if (!fallbackUrl.empty()) {
        Quote(fallbackUrl, "");
        intent << TStringBuf(";S.browser_fallback_url=") << fallbackUrl;
    }
    intent << TStringBuf(";end");

    if (clientInfo.IsNavigator()) {
        Quote(intent, "");
        return TStringBuilder() << TStringBuf("vins://open_url_with_fallback?url=") << intent;
    }

    return intent;
}

TString CleanPhone(TStringBuf phone, bool normalize) {
    TString phoneClean(Reserve(phone.size()));
    for (size_t i = 0, size = phone.size(); i < size; ++i) {
        char ch = phone[i];
        if (!normalize || (ch >= '0' && ch <= '9'))
            phoneClean.push_back(ch);
    }
    return phoneClean;
}

void AddSearchCgi(NGeobase::TId lr, EContentSettings restrictionLevel, TStringBuf lang, TStringBuf query, TCgiParameters& cgi) {
    cgi.InsertUnescaped(TStringBuf("text"), query);
    if (IsValidId(lr)) {
        cgi.InsertUnescaped(TStringBuf("lr"), ToString(lr));
    } else {
        // если не удалось определить lr, то не шлём его совсем
        // в этом случае это не страшно, т.к. это ссылка для браузера/web view,
        // и lr там определится на общих основаниях
    }
    if (restrictionLevel == EContentSettings::children) {
        cgi.InsertUnescaped(TStringBuf("fyandex"), TStringBuf("1"));
    }
    if (lang) {
        cgi.InsertUnescaped(TStringBuf("l10n"), lang);
    }
}

bool NeedAddPlusPrefix(TStringBuf cleanedPhone) {
    bool notShort = cleanedPhone.size() >= 5;
    bool  notRussianLocalFormat = (cleanedPhone[0] != '8');

    return notShort && notRussianLocalFormat;
}

} // namespace

TString PrintCgi(const TCgiParameters& cgi) {
    TString cgiStr = cgi.Print();
    SubstGlobal(cgiStr, "+", "%20"); // Some strange decoding of '+' symbol in mobile maps
    return cgiStr;
}

TString GenerateMapsUri(const TClientInfo& clientInfo, const TUserLocation& location, const TStringBuf path, const TCgiParameters& cgi,
        const TStringBuf privateNavigatorKey, bool supportsIntentUrls, bool needSimpleUrl, const TStringBuf fallbackUrl) {
    TStringBuilder baseUrl;
    baseUrl << TStringBuf("yandex.") << location.UserTld() << '/'
            << (location.UserTld() == TStringBuf("com.tr") ? TStringBuf("harita") : TStringBuf("maps"))
            << path;

    TString cgiStr = PrintCgi(cgi);

    TStringBuilder link;
    link << baseUrl << '?' << cgiStr;

    TStringBuilder simpleUrl;
    simpleUrl << TStringBuf("https://") << link;

    SignGeoUri(privateNavigatorKey, MAPS_URL_PREFIX, link);

    if (needSimpleUrl || !supportsIntentUrls) {
        // Default url
        return simpleUrl;
    }

    TString simpleUrlEncoded = simpleUrl;
    Quote(simpleUrlEncoded, "");

    TStringBuilder browserFallbackLink;
    browserFallbackLink << TStringBuf("browser://?url=")
                        << simpleUrlEncoded;

    TStringBuilder intentLink;
    if (clientInfo.IsIOS()) {
        TStringBuilder intentUrl;
        intentUrl << MAPS_URL_PREFIX << TStringBuf("://") << link;
        return BuildIOsIntent(clientInfo, intentUrl,
                              fallbackUrl ? TString{fallbackUrl} : (clientInfo.IsSearchApp() ? browserFallbackLink : simpleUrl));
    }
    // encode browser link twice to send all parameters
    Quote(browserFallbackLink, "");

    return BuildAndroidIntent(clientInfo, "yandexmaps", "ru.yandex.yandexmaps",
                              link, fallbackUrl ? TString{fallbackUrl} : ((clientInfo.IsYaBrowser() ? simpleUrl : browserFallbackLink)));
}

TString GenerateNavigatorUri(const TClientInfo& clientInfo, const TString& privateNavigatorKey, const TString& urlPrefix,
                             const TCgiParameters& cgi, const TString& fallbackUrl, bool supportsIntentUrls,
                             bool supportsNavigator)
{
    if (!supportsIntentUrls) {
        return fallbackUrl;
    }

    TStringBuilder navigatorUri;
    navigatorUri << urlPrefix << (urlPrefix.back() == '?' ? "" : "?")
                 << cgi.Print();

    if (supportsNavigator) {
        return TStringBuilder() << NAVIGATOR_URL_PREFIX << TStringBuf("://") << navigatorUri;
    }

    SignGeoUri(privateNavigatorKey, NAVIGATOR_URL_PREFIX, navigatorUri);
    if (clientInfo.IsIOS()) {
        TStringBuilder intentUrl;
        intentUrl << NAVIGATOR_URL_PREFIX << TStringBuf("://") << navigatorUri;
        return BuildIOsIntent(clientInfo, intentUrl, fallbackUrl);
    }

    TString fallbackUrlEncoded = fallbackUrl;
    Quote(fallbackUrlEncoded, "");
    return BuildAndroidIntent(clientInfo, "yandexnavi", "ru.yandex.yandexnavi",
                              navigatorUri, fallbackUrlEncoded);
}

TString GenerateTaxiUri(const TClientFeatures& clientFeatures, const NSc::TValue& from, const NSc::TValue& to, bool useFallback) {
    const NSc::TValue& fromLocation = from.Get("location");
    const NSc::TValue& toLocation = to.Get("location");
    const NSc::TValue& fromAddressLine = from.Has("address_line") ? from["address_line"] : from.TrySelect("geo/address_line");
    const NSc::TValue& toAddressLine = to.Has("address_line") ? to["address_line"] : to.TrySelect("geo/address_line");

    TCgiParameters appTaxiCgi;
    TCgiParameters webTaxiCgi;

    AddTaxiLoggingParams(&appTaxiCgi);
    AddTaxiLoggingParams(&webTaxiCgi);

    if (!fromLocation.IsNull()) {
        appTaxiCgi.InsertUnescaped("start-lat", ToString(fromLocation["lat"].GetNumber()));
        appTaxiCgi.InsertUnescaped("start-lon", ToString(fromLocation["lon"].GetNumber()));
    }
    if (!fromAddressLine.IsNull()) {
        webTaxiCgi.InsertUnescaped("gfrom", fromAddressLine.GetString());
    }

    if (!toLocation.IsNull()) {
        appTaxiCgi.InsertUnescaped("end-lat", ToString(toLocation["lat"].GetNumber()));
        appTaxiCgi.InsertUnescaped("end-lon", ToString(toLocation["lon"].GetNumber()));
    }
    if (!toAddressLine.IsNull()) {
        webTaxiCgi.InsertUnescaped("gto", toAddressLine.GetString());
    }

    TStringBuilder taxiWebUrl;
    taxiWebUrl << TStringBuf("https://taxi.yandex.ru");
    if (!webTaxiCgi.empty()) {
        taxiWebUrl << "/?" << webTaxiCgi.Print();
    }

    if (clientFeatures.SupportsIntentUrls()) {
        TStringBuilder taxiUri;
        if (!appTaxiCgi.empty()) {
            taxiUri << TStringBuf("route?") << appTaxiCgi.Print();
        }

        TStringBuilder browserFallback;
        if (useFallback) {
            TString simpleFallback = taxiWebUrl;
            Quote(simpleFallback, "");
            browserFallback << TStringBuf("browser://?url=") << simpleFallback;
        } else if (clientFeatures.IsIOS()) {
            browserFallback << TStringBuf("https://itunes.apple.com/app/rider/id472650686");
        }

        if (clientFeatures.IsIOS()) {
            TStringBuilder intentUrl;
            intentUrl << TStringBuf("yandextaxi://") << taxiUri;
            return BuildIOsIntent(clientFeatures, intentUrl,
                                  clientFeatures.IsSearchApp() ? browserFallback : taxiWebUrl);
        } else if (clientFeatures.IsAndroid()) {
            // encode browser link twice to send all parameters
            Quote(browserFallback, "");
            return BuildAndroidIntent(clientFeatures, "yandextaxi", "ru.yandex.taxi",
                                      taxiUri, (clientFeatures.IsSearchApp() ? browserFallback : taxiWebUrl));
        }
    }

    return taxiWebUrl;
}

const NSc::TValue& GetAppleSchemes() {
    return AppleSchemes;
}

TString GenerateApplicationUri(const TClientFeatures& clientFeatures, TStringBuf app, TStringBuf fallbackUrl) {
    return GenerateApplicationUri(clientFeatures.SupportsIntentUrls(), clientFeatures, app, fallbackUrl);
}

TString GenerateApplicationUri(const bool supportsIntentUrls, const TClientInfo& clientInfo, TStringBuf app, TStringBuf fallbackUrl) {
    TString fb(fallbackUrl);
    if (supportsIntentUrls && !app.empty()) {
        if (clientInfo.IsIOS()) {
            const NSc::TValue appleSchemes = GetAppleSchemes();
            if (appleSchemes.Has(app)) {
                TStringBuilder intentUrl;
                intentUrl << appleSchemes[app].GetString();
                return BuildIOsIntent(clientInfo, intentUrl, fb);
            }
        } else {
            return BuildAndroidIntent(clientInfo, "" /* scheme */, app /* package */, "" /* intent */, fb);
        }
    }
    return fb;
}

TString GenerateAuthorizationUri(const bool supportsOpenYandexAuth) {
    if (supportsOpenYandexAuth) {
        return TString(SEARCH_APP_AUTH_URI);
    }
    return {};
}

TString GenerateMusicVerticalUri(const TClientFeatures& clientFeatures) {
    return GenerateMusicVerticalUri(clientFeatures.SupportsMusicSDKPlayer(), clientFeatures);
}

TString GenerateMusicVerticalUri(const bool hasMusicSdkClient, const TClientInfo& clientInfo) {
    if (hasMusicSdkClient && !clientInfo.IsYaMusic()) {
        return "https://music.yandex.ru/pptouch";
    }
    return {};
}

TString GenerateMusicAppUri(const TClientFeatures& clientFeatures, EMusicUriType uriType, TStringBuf path, TCgiParameters cgi) {
    return GenerateMusicAppUri(clientFeatures.SupportsIntentUrls(), clientFeatures, uriType, path, cgi);
}

TString GenerateMusicAppUri(const bool supportsIntentUrls, const TClientInfo& clientInfo, EMusicUriType uriType, TStringBuf path, TCgiParameters cgi) {
    cgi.InsertUnescaped(TStringBuf("from"), TStringBuf("alice"));
    const TString cgiStr = TStringBuilder() << '?' << PrintCgi(cgi);

    // Remove stub for installing the app.
    cgi.InsertUnescaped(TStringBuf("mob"), TStringBuf("0"));
    if (cgi.Has("play") && cgi.Get("play") == TStringBuf("true")) {
        cgi.ReplaceUnescaped("play", "1");
    }

    TStringBuf schema = "yandexmusic";
    TStringBuf pkg = "ru.yandex.music";

    TStringBuilder fallbackUrl;
    bool isRadio = true;
    if (uriType == EMusicUriType::Music) {
        fallbackUrl << "https://music.yandex.ru/" << path << '?' << PrintCgi(cgi);
        isRadio = false;
    } else if (uriType == EMusicUriType::RadioPersonal && !clientInfo.IsYaAuto()) {
        fallbackUrl << "https://radio.yandex.ru/user/onyourwave?" << PrintCgi(cgi);
    } else {
        fallbackUrl << "https://radio.yandex.ru/" << path << '?' << PrintCgi(cgi);
    }

    if (supportsIntentUrls && (!clientInfo.IsYaAuto())) {
        TString appPath(isRadio ? TString::Join("radio/", path) : path);
        if (clientInfo.IsIOS()) {
            TStringBuilder intentUrl;
            intentUrl << schema << TStringBuf("://") << appPath << cgiStr;
            return BuildIOsIntent(clientInfo, intentUrl, fallbackUrl);
        } else {
            return BuildAndroidIntent(clientInfo, schema, pkg, appPath+cgiStr, fallbackUrl);
        }
    }
    return fallbackUrl;
}

TString GeneratePhoneUri(const TClientInfo& clientInfo, const TString& phone, bool normalize, bool addPrefix) {
    return GeneratePhoneUri(clientInfo, TStringBuf(phone), normalize, addPrefix);
}

TString GeneratePhoneUri(const TClientInfo& clientInfo, TStringBuf phone, bool normalize, bool addPrefix) {
    TString phoneClean = CleanPhone(phone, normalize);
    bool needPlus = addPrefix && NeedAddPlusPrefix(phoneClean);

    TStringBuf plus = needPlus ? TStringBuf("+") : TStringBuf("");

    if (clientInfo.IsNavigator()) {
        return TStringBuilder() << NAVIGATOR_URL_PREFIX << "://dial_phone?phone_number=" << plus << phoneClean;
    } else if (clientInfo.IsTouch()) {
        return TStringBuilder() << TStringBuf("tel:") << plus << phoneClean;
    } else if (clientInfo.IsChatBot()) {
        return TStringBuilder() << plus << phoneClean;
    }

    return phoneClean;
}

TString GenerateSearchUri(const TClientInfo& clientInfo, const TUserLocation& location, EContentSettings restrictionLevel,
                          TStringBuf query, bool canOpenLinkSearchViewport, TCgiParameters cgi, bool forceViewport)
{
    if (!cgi.Has("query_source")){
        cgi.InsertEscaped("query_source", "alice");
    }
    TStringBuilder url;
    if (canOpenLinkSearchViewport || forceViewport) {
        url << "viewport://?";
        cgi.InsertUnescaped(TStringBuf("viewport_id"), TStringBuf("serp"));
        cgi.InsertUnescaped(TStringBuf("noreask"), TStringBuf("1"));
    } else if (clientInfo.IsTouch()) {
        url << "https://yandex." << location.UserTld() << "/search/touch/?";
    } else {
        url << "https://yandex." << location.UserTld() << "/search/?";
    }
    AddSearchCgi(location.UserRegion(), restrictionLevel, clientInfo.Lang, query, cgi);
    url << PrintCgi(cgi);
    return url;
}

TString GenerateSearchAdsUri(const TClientInfo& clientInfo, const TUserLocation& location, EContentSettings restrictionLevel,
                             TStringBuf query, TCgiParameters cgi)
{
    if (!cgi.Has("query_source")){
        cgi.InsertEscaped("query_source", "alice");
    }
    TStringBuilder url;
    if (clientInfo.IsTouch()) {
        url << "https://yandex." << location.UserTld() << "/search/touch/ads?";
    } else {
        url << "https://yandex." << location.UserTld() << "/search/ads?";
    }
    AddSearchCgi(location.UserRegion(), restrictionLevel, clientInfo.Lang, query, cgi);
    url << PrintCgi(cgi);
    return url;
}

TString GenerateObjectUri(const TClientInfo& clientInfo, const TUserLocation& location, EContentSettings restrictionLevel,
                          const TString& objectId, const TString& objectName, const TString& seoName, bool canOpenLinkSearchViewport)
{
    TStringBuilder objectUri;
    if (clientInfo.IsSearchApp()) {
        TCgiParameters cgi;
        cgi.InsertEscaped(TStringBuf("faf"), TStringBuf("adr"));
        cgi.InsertEscaped(TStringBuf("uri"), TStringBuf("ymapsbm1://org?oid=") + objectId);
        objectUri << GenerateSearchUri(clientInfo, location, restrictionLevel, objectName, canOpenLinkSearchViewport, cgi);
    } else {
        // Default uri
        objectUri << TStringBuf("https://yandex.") << location.UserTld() << TStringBuf("/maps/org/");
        if (!seoName.empty()) {
            objectUri << seoName << "/";
        }
        objectUri << objectId;
    }
    return objectUri;
}

TString GenerateObjectCatalogUri(const TUserLocation& location, const TString& objectId,
                                 bool reviewsIntent, bool photoIntent)
{
    TStringBuilder objectUri;
    TCgiParameters cgi;

    objectUri << "https://yandex." << location.UserTld() << "/profile/" << objectId << "?";

    NGeobase::TId lr = location.UserRegion();
    if (IsValidId(lr)) {
       cgi.InsertUnescaped(TStringBuf("lr"), ToString(lr));
    }
    if (reviewsIntent) {
        cgi.InsertUnescaped(TStringBuf("intent"), TStringBuf("reviews"));
    }

    if (photoIntent) {
        cgi.InsertUnescaped(TStringBuf("intent"), TStringBuf("photo"));
    }

    objectUri << PrintCgi(cgi);

    return objectUri;
}

TString GenerateNewsUri(const TClientInfo& ci, TStringBuf dirtyUrl) {
    constexpr TStringBuf urlParamPrefix = "url=";

    TString url;
    NUri::TUri uri;
    if (uri.Parse(dirtyUrl, NUri::TUri::TFeature::FeaturesRecommended) == NUri::TUri::ParsedOK) {
        if (uri.GetScheme() == NUri::TScheme::EKind::SchemeEmpty) {
            uri.SetScheme(NUri::TScheme::EKind::SchemeHTTPS);
        }
        url = uri.PrintS();
    } else {
        url = ToString(dirtyUrl);
    }

    if (!ci.IsSearchApp()) {
        if (url.StartsWith(TStringBuf("yellowskin"))) {
            TStringBuf tmpUrl(url);
            while (TStringBuf tok = tmpUrl.NextTok('&')) {
                if (tok.StartsWith(urlParamPrefix)) {
                    return PatchNewsUrl(UrlUnescapeRet(tok.Skip(urlParamPrefix.size())));
                }
            }
        } else {
            return PatchNewsUrl(url);
        }
    }
    return url;
}

TString GenerateWeatherUri(const TClientInfo& clientInfo, TStringBuf origUri, ui16 mday) {
    TStringBuilder weatherLink;

    {
        const bool isTouch = clientInfo.IsTouch();
        TCgiParameters cgi;
        if (clientInfo.IsSearchApp()) {
            cgi.InsertUnescaped(TStringBuf("appsearch_header"), TStringBuf("1"));
            cgi.InsertUnescaped(TStringBuf("appsearch_ys"), TStringBuf("2"));
        }

        weatherLink << origUri;
        if (!isTouch) {
            weatherLink << TStringBuf("/details");
        }

        if (cgi) {
            weatherLink << '?' << cgi.Print();
        }

        weatherLink << (cgi ? '&' : '?')
                    << "utm_source=alice&utm_campaign=card";

        if (!isTouch) {
            weatherLink << '#';
        }
        else {
            weatherLink << TStringBuf("#d_");
        }

        weatherLink << static_cast<unsigned int>(mday);
    }

    TStringBuilder uri;
    if (clientInfo.IsSearchApp()) {
        TCgiParameters cgiSearch;
        cgiSearch.InsertEscaped(TStringBuf("primary_color"), TStringBuf("#ffffff"));
        cgiSearch.InsertEscaped(TStringBuf("secondary_color"), TStringBuf("#000000"));
        cgiSearch.InsertEscaped(TStringBuf("url"), weatherLink);
        uri << TStringBuf("yellowskin://?") << cgiSearch.Print();
    }
    else {
        uri.swap(weatherLink);
    }

    return uri;
}

TString GenerateMapsTrafficUri(const TClientInfo& clientInfo, const TUserLocation& userLocation,
                               const TString& privateNavigatorKey, const NGeobase::TLookup& geobase, NGeobase::TId regionId,
                               bool supportsIntentUrls, bool supportsNavigator)
{
    if (!IsValidId(regionId)) {
        return TString();
    }
    NGeobase::TRegion region = geobase.GetRegionById(regionId);
    TString locationStr = TString::Join(ToString(region.GetLongitude()), ",", ToString(region.GetLatitude()));

    TCgiParameters cityCgi;
    cityCgi.InsertUnescaped(TStringBuf("zoom"), TStringBuf("10"));
    cityCgi.InsertUnescaped(TStringBuf("no-balloon"), TStringBuf("1"));
    cityCgi.InsertUnescaped(TStringBuf("lat"), ToString(region.GetLatitude()));
    cityCgi.InsertUnescaped(TStringBuf("lon"), ToString(region.GetLongitude()));

    if (supportsNavigator) {
        return TStringBuilder() << NAVIGATOR_URL_PREFIX << TStringBuf("://show_point_on_map?") << cityCgi.Print();
    } else {
        NGeobase::TLinguistics names = geobase.GetLinguistics(regionId, clientInfo.Locale.Lang);
        TCgiParameters cgi;
        cgi.InsertEscaped(TStringBuf("text"), names.NominativeCase);
        cgi.InsertUnescaped(TStringBuf("ll"), locationStr);
        cgi.InsertUnescaped(TStringBuf("oll"), locationStr);
        cgi.InsertUnescaped(TStringBuf("ol"), TStringBuf("geo"));
        cgi.InsertUnescaped(TStringBuf("l"), TStringBuf("trf"));

        TString simpleMapUrl = GenerateMapsUri(clientInfo, userLocation, "", cgi, privateNavigatorKey, supportsIntentUrls, false);
        return GenerateNavigatorUri(clientInfo, privateNavigatorKey, "show_point_on_map", cityCgi, simpleMapUrl,
            supportsIntentUrls, supportsNavigator);
    }
}

void InitAppleSchemes() {
    TString content;
    if (!NResource::FindExact("apple_schemes.json", &content)) {
        ythrow yexception() << "Unable to load built-in resource 'apple_schemes.json'";
    }

    NSc::TValue json = NSc::TValue::FromJson(content);
    AppleSchemes.Swap(json);
}

TString GenerateSimilarImagesSearchUrl(const TClientInfo& clientInfo, TStringBuf cbirId,
                                       TStringBuf aliceSource, TStringBuf cbirPage, TStringBuf report,
                                       bool needNextPage, bool disablePtr, int cropId) {
    TCgiParameters cgi;
    cgi.InsertUnescaped(cropId >= 0 ? TStringBuf("base_cbir_id") : TStringBuf("cbir_id"), cbirId);
    cgi.InsertUnescaped(TStringBuf("rpt"), report);
    cgi.InsertUnescaped(TStringBuf("lang"), clientInfo.Locale.ToString());
    cgi.InsertUnescaped(TStringBuf("l10n"), clientInfo.Locale.Lang);
    cgi.InsertUnescaped(TStringBuf("uuid"), clientInfo.Uuid);
    cgi.InsertUnescaped(TStringBuf("app_id"), clientInfo.ClientId);
    if (report == "imageview") {
        cgi.InsertUnescaped(TStringBuf("native"), TStringBuf("0"));
    }
    if (!aliceSource.empty()) {
        cgi.InsertUnescaped(TStringBuf("alice_source"), aliceSource);
    }
    if (!cbirPage.empty()) {
        cgi.InsertUnescaped(TStringBuf("cbir_page"), cbirPage);
    }
    if (needNextPage) {
        cgi.InsertUnescaped(TStringBuf("p"), TStringBuf("1"));
    }
    if (cropId >= 0) {
        cgi.InsertUnescaped(TStringBuf("crop_id"), ToString(cropId));
    }

    TStringBuilder resultUrl;
    resultUrl << TStringBuf("https://yandex.ru/images/touch/search?") << PrintCgi(cgi);

    if (clientInfo.IsSearchApp()) {
        TStringBuilder resultYellowskinUrl;
        TCgiParameters cgiYellowskin;
        cgiYellowskin.InsertEscaped(TStringBuf("url"), resultUrl);
        if (disablePtr) {
            cgiYellowskin.InsertUnescaped(TStringBuf("ptr_disabled"), TStringBuf("1"));
        }
        resultYellowskinUrl << TStringBuf("yellowskin://?") << cgiYellowskin.Print();
        return resultYellowskinUrl;
    }

    return resultUrl;
}


TString GenerateSimilarsGalleryLink(const TClientInfo& clientInfo, TStringBuf cbirId, TStringBuf imgUrl) {
    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("img_url"), imgUrl);
    cgi.InsertUnescaped(TStringBuf("cbir_id"), cbirId);
    cgi.InsertUnescaped(TStringBuf("rpt"), TStringBuf("imageview"));
    cgi.InsertUnescaped(TStringBuf("l10n"), clientInfo.Locale.Lang);
    cgi.InsertUnescaped(TStringBuf("uuid"), clientInfo.Uuid);
    cgi.InsertUnescaped(TStringBuf("alice_source"), TStringBuf("similar"));
    cgi.InsertUnescaped(TStringBuf("native"), TStringBuf("0"));

    TStringBuilder resultUrl;
    resultUrl << TStringBuf("https://yandex.ru/images/touch/search?") << PrintCgi(cgi);

    if (clientInfo.IsSearchApp()) {
        TStringBuilder resultYellowskinUrl;
        TCgiParameters cgiYellowskin;
        cgiYellowskin.InsertEscaped(TStringBuf("url"), resultUrl);
        resultYellowskinUrl << TStringBuf("yellowskin://?") << cgiYellowskin.Print();
        return resultYellowskinUrl;
    }

    return resultUrl;
}

TString GenerateMarketDealsLink(TStringBuf cbirId, TStringBuf uuid) {
    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("cbir_id"), cbirId);
    cgi.InsertUnescaped(TStringBuf("clid"), TStringBuf("900"));
    if (!uuid.empty()) {
        cgi.InsertUnescaped(TStringBuf("uuid"), uuid);
    }
    cgi.InsertUnescaped(TStringBuf("source"), TStringBuf("alice"));

    TStringBuilder resultUrl;
    resultUrl << TStringBuf("https://m.market.yandex.ru/picsearch?") << PrintCgi(cgi);

    return resultUrl;
}

TString GenerateTranslateUri(const TClientFeatures& clientFeatures, TStringBuf tld, TStringBuf text, TStringBuf dir) {
    return GenerateTranslateUri(clientFeatures.SupportsIntentUrls(), clientFeatures, tld, text, dir);
}

TString GenerateTranslateUri(const bool supportsIntentUrls, const TClientInfo& clientInfo, TStringBuf tld, TStringBuf text, TStringBuf dir) {
    TStringBuilder webUrl;
    webUrl << TStringBuf("https://translate.yandex.") << (tld.empty() ? "ru" : tld);

    TCgiParameters cgi;
    cgi.InsertEscaped(TStringBuf("text"), text);
    cgi.InsertEscaped(TStringBuf("lang"), dir);
    webUrl << "/?" << PrintCgi(cgi);

    TCgiParameters additionalCgi;
    additionalCgi.InsertUnescaped(TStringBuf("searchapp_from_source"), TStringBuf("alice"));

    if (supportsIntentUrls) {
        TString webUrlEncoded = webUrl;
        Quote(webUrlEncoded, "");

        TStringBuilder browserFallbackLink;
        browserFallbackLink << TStringBuf("browser://?url=") << webUrlEncoded;

        if (clientInfo.IsIOS()) {
            if (clientInfo.IsSearchApp() && clientInfo.IsIOSAppOfVersionOrNewer(27, 0)) {
                webUrl << "&" << PrintCgi(additionalCgi);
                return webUrl;
            }
            TStringBuilder intentUrl;
            intentUrl << TStringBuf("yandextranslate://?") << PrintCgi(cgi);
            return BuildIOsIntent(clientInfo, intentUrl,
                                  clientInfo.IsSearchApp() ? browserFallbackLink : webUrl);
        } else if (clientInfo.IsAndroid()) {
            if (clientInfo.IsSearchApp() && clientInfo.IsAndroidAppOfVersionOrNewer(9, 0)) {
                webUrl << "&" << PrintCgi(additionalCgi);
                return webUrl;
            } else {
                Quote(browserFallbackLink, "");
                return BuildAndroidIntent(clientInfo, "yandextranslate", "ru.yandex.translate",
                                          "translate?" + PrintCgi(cgi), clientInfo.IsSearchApp() ? browserFallbackLink : webUrl);
            }
        }
    }

    return webUrl;
}

TString GenerateProductsSearchUri(const TStringBuf query, const bool canOpenLinkSearchViewPort) {
    TCgiParameters cgi;
    TStringBuilder url;
    cgi.InsertEscaped("query_source", "alice");
    // temporary solution (wait for fix -  HOME-77401)
    cgi.InsertEscaped("query_source", "voice");
    if (canOpenLinkSearchViewPort) {
        url << "viewport://?";
        cgi.InsertUnescaped(TStringBuf("viewport_id"), TStringBuf("products"));
        cgi.InsertUnescaped(TStringBuf("noreask"), TStringBuf("1"));
    } else {
        url << "https://yandex.ru/products/search?";
    }
    cgi.InsertUnescaped(TStringBuf("text"), query);
    url << PrintCgi(cgi);
    return url;
}

TString GenerateMessengerUri(TStringBuf phone, TStringBuf client, TStringBuf text, TStringBuf fallback) {
    TString phoneClean = CleanPhone(phone, /* normalize */ true);
    bool needPlus = NeedAddPlusPrefix(phoneClean);

    TStringBuf plus = needPlus ? "+" : "";

    TString escapedText = TString{text};
    Quote(escapedText, "");

    TStringBuilder builder;
    if (client == "sms")
        builder << "sms://" << plus << phoneClean << "?sms_body=" << escapedText;
    if (client == "whatsapp")
        builder << "msg://com.whatsapp?text=" << escapedText << "&name=" << phoneClean;

    if (builder.empty())
        return {};

    builder << "&fallback=" << fallback;
    return builder;
}

TClientActionUrl::TClientActionUrl(EType type)
    : Type(type)
{
}

TClientActionUrl::TClientActionUrl(EType type, NSc::TValue payload)
    : Type(type)
    , Payload(payload.Clone())
{
}

TString TClientActionUrl::ToString() const {
    return CreateClientActionUrl([this](TAddActionCallback addAction) { addAction(Type, Payload); });
}

// static
TString TClientActionUrl::ToString(const TVector<TClientActionUrl>& actions) {
    auto cb = [&actions](TAddActionCallback addAction) {
        for (const auto& action : actions) {
            addAction(action.Type, action.Payload);
        }
    };
    return CreateClientActionUrl(cb);
}

// static
TString TClientActionUrl::OpenDialogById(TStringBuf dialogId, const TMaybe<NSc::TValue>& maybePayload) {
    NSc::TValue payload;
    if (maybePayload) {
        payload = maybePayload->Clone();
    }
    NSc::TValue& serverAction = payload["directives"].Push();
    serverAction["type"] = TStringBuf("server_action");
    serverAction["name"] = TStringBuf("new_dialog_session");
    serverAction["payload"]["dialog_id"] = dialogId;

    return CreateClientActionDialog(TStringBuf("dialog_id"), dialogId, EType::OpenDialog, payload);
}

// static
TString TClientActionUrl::OpenDialogByText(TStringBuf text, const TMaybe<NSc::TValue>& payload) {
    return CreateClientActionDialog(TStringBuf("text"), text, EType::Text, payload);
}

namespace {

bool AddUtmReferrer(const TClientInfo& client, TCgiParameters* cgi) {
    auto quote = [](TString toQuote) {
        Quote(toQuote, ""/* safe chars */);
        return toQuote;
    };

    static const TString REFERRER_SEARCHAPP = quote("https://yandex.ru/searchapp?from=alice&text=");
    static const TString REFERRER_YABRO = quote("https://yandex.ru/?from=alice");

    Y_ASSERT(cgi);

    constexpr TStringBuf utmReferrer = "utm_referrer";

    if (client.IsSearchApp()) {
        cgi->ReplaceUnescaped(utmReferrer, REFERRER_SEARCHAPP);
        return true;
    }
    if (client.IsYaBrowser()) {
        cgi->ReplaceUnescaped(utmReferrer, REFERRER_YABRO);
        return true;
    }
    return false;
}

} // namespace

TString AddUtmReferrer(const TClientInfo& client, TStringBuf url) {
    if (url.Contains("gmail") || // ASSISTANT-2813
        url.Contains("hp.") ||   // ASSISTANT-2954
        url.Contains("cbr."))    // ALICE-1280
    {
        return TString{url};
    }

    NUri::TUri uri;
    NUri::TState::EParsed uriResult = uri.Parse(url, NUri::TFeature::FeaturesDefault | NUri::TFeature::FeatureSchemeFlexible);

    if (uriResult != NUri::TState::EParsed::ParsedOK) {
        return TString{url};
    }

    TStringBuf scheme = uri.GetField(NUri::TField::FieldScheme);
    if (scheme != TStringBuf("http") && scheme != TStringBuf("https")) {
        return TString{url};
    }

    TCgiParameters cgi(uri.GetField(NUri::TField::FieldQuery));
    if (!AddUtmReferrer(client, &cgi)) {
        return TString{url};
    }

    const TString cgiStr = cgi.Print(); // this string must live until ~TUriUpdate() call
    NUri::TUriUpdate(uri).Set(NUri::TField::FieldQuery, cgiStr);
    return uri.PrintS();
}

} // namespace NAlice
