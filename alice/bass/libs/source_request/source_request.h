#pragma once

#include <alice/bass/libs/config/config.sc.h>

#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/metrics/place.h>

#include <alice/library/client/client_features.h>
#include <alice/library/experiments/flags.h>

#include <library/cpp/scheme/domscheme_traits.h>

#include <util/generic/maybe.h>

class TConfig;

using TCustomResponseCallback = std::function<NHttpFetcher::TFetchStatus(const TString&)>;

class ISourcesRegistryDelegate {
public:
    virtual ~ISourcesRegistryDelegate() = default;
    virtual TMaybe<TString> GetTVM2ServiceTicket(TStringBuf serviceId) const = 0;
};

class TDummySourcesRegistryDelegate : public ISourcesRegistryDelegate {
public:
    TMaybe<TString> GetTVM2ServiceTicket(TStringBuf /* serviceId */) const override {
        return {};
    }
};

/**
 * Placeholder for an external source config accessor, metrics and other related things
 */
struct TSourceContext : private TNonCopyable {
    using TSourceConst = NBASSConfig::TConfigConst<TSchemeTraits>::TSourceConst;
    using TConfigAccessor = std::function<TSourceConst(const TConfig&)>;

    TSourceContext(const TString& source, NBASS::NMetrics::ICountersPlace& counters, TConfigAccessor configAccessor);
    TSourceContext(const TString& source, const TString& requestType, NBASS::NMetrics::ICountersPlace& counters, TConfigAccessor configAccessor);
    TSourceContext(TSourceContext&& sourceContext);

    TConfigAccessor ConfigAccessor;
    NHttpFetcher::IMetrics::TRef Metrics;
    const TString Source;
    const TMaybe<TString> RequestType;
};

/**
 * Wrapper over external source context for creating HTTP request to it
 */
class TSourceRequestFactory {
public:
    TSourceRequestFactory(const TSourceContext& context, const TConfig& config,
                          TMaybe<TStringBuf> path, const ISourcesRegistryDelegate& sourcesRegistryDelegate,
                          const NAlice::TExpFlags& expFlags = Default<NAlice::TExpFlags>());

    void OverrideUri(TStringBuf uri) {
        UriOverride = uri;
    }

    NHttpFetcher::TRequestPtr Request() const;
    NHttpFetcher::TRequestPtr Request(TCustomResponseCallback callback) const; // callback will be called in the same thread during THandle::Wait() call

    NHttpFetcher::TRequestPtr AttachRequest(NHttpFetcher::IMultiRequest::TRef multiRequest) const;
    NHttpFetcher::TRequestPtr AttachRequest(NHttpFetcher::IMultiRequest::TRef multiRequest, TCustomResponseCallback callback) const;

    NHttpFetcher::TRequestPtr MakeOrAttachRequest(NHttpFetcher::IMultiRequest::TRef multiRequest);
    NHttpFetcher::TRequestPtr MakeOrAttachRequest(NHttpFetcher::IMultiRequest::TRef multiRequest, TCustomResponseCallback callback);

private:
    NUri::TUri RequestUrl() const;
    void FillAdditionalFields(NHttpFetcher::TRequest* request) const;

private:
    const TSourceContext& Context;
    const TConfig& Config;
    const TMaybe<TString> AppendPath;
    const ISourcesRegistryDelegate& SourcesRegistryDelegate;
    const NAlice::TExpFlags& ExpFlags;
    TMaybe<TString> UriOverride;
};

/**
 * Placeholder for external source contexts.
 */
class TSourcesRegistry {
public:
    // Initializes source contexts in .cpp (cannot do it here due to necessary forward declaration of TConfig)
    TSourcesRegistry(NBASS::NMetrics::ICountersPlace& counters,
                     const ISourcesRegistryDelegate& sourcesRegistryDelegate);

private:
    TVector<TString> Registry{};

public:
    const ISourcesRegistryDelegate& SourcesRegistryDelegate;


    TSourceContext AbuseApi;
    TSourceContext Afisha;
    TSourceContext AliceGraph;
    TSourceContext ApiMapsStaticMapRouter;
    TSourceContext AviaBackend;
    TSourceContext AviaPriceIndex;
    TSourceContext AviaPriceIndexMinPrice;
    TSourceContext AviaSuggests;
    TSourceContext AviaTDApiInit;

    TSourceContext BlackBox;
    TSourceContext BlackBoxTest;

    TSourceContext CalendarApi;
    TSourceContext CarRoutes;
    TSourceContext CarsharingGreetingPhrase;
    TSourceContext CloudApiDisk;
    TSourceContext CloudApiDiskUpload;
    TSourceContext ComputerVision;
    TSourceContext ComputerVisionClothes;
    TSourceContext ComputerVisionCbirFeatures;
    TSourceContext Convert;

    TSourceContext EntitySearch;
    TSourceContext ExternalSkillsApi;
    TSourceContext ExternalSkillsDb;
    TSourceContext ExternalSkillsKvSaaS;
    TSourceContext ExternalSkillsRecommender;
    TSourceContext ExternalSkillsSaaS;

    TSourceContext GeneralConversationTurkish;

    TSourceContext GeoCoderLl2Geo;
    TSourceContext GeoCoderTextToRoadName;
    TSourceContext GeoMetaSearchOrganization;
    TSourceContext GeoMetaSearchResolveText;
    TSourceContext GeoMetaSearchResolveTextNextPage;
    TSourceContext GeoMetaSearchReverseResolve;
    TSourceContext GeoMetaSearchRoutePointContext;

    TSourceContext MapsInfoExportBackground;
    TSourceContext Market;
    TSourceContext MarketBlue;
    TSourceContext MarketBlueHeavy;
    TSourceContext MarketCheckouter;
    TSourceContext MarketCheckouterHeavy;
    TSourceContext MarketCheckouterIntervals;
    TSourceContext MarketCheckouterOrders;
    TSourceContext MarketFormalizer;
    TSourceContext MarketHeavy;
    TSourceContext MarketMds;
    TSourceContext MarketStockStorage;
    TSourceContext MarketPersBasket;
    TSourceContext MarketBeruMyBonusesList;
    TSourceContext MassTransitRoutes;
    TSourceContext Music;
    TSourceContext MusicCatalog;
    TSourceContext MusicCatalogBulk;
    TSourceContext MusicQuasar;
    TSourceContext MusicSuggests;
    TSourceContext MusicAvatarsColor;

    TSourceContext NerApi;
    TSourceContext News;
    TSourceContext NewsApi;
    TSourceContext NewsApiScheduler;
    TSourceContext NormalizedQuery;

    TSourceContext Passport;
    TSourceContext PedestrianRoutes;
    TSourceContext PersonalData;
    TSourceContext PersonalDataTest;

    TSourceContext QuasarBillingContentBuy;
    TSourceContext QuasarBillingContentPlay;
    TSourceContext QuasarBillingPromoAvailability;
    TSourceContext QuasarBillingSkills;

    TSourceContext RadioStream;
    TSourceContext RadioStreamAvailableStations;
    TSourceContext RadioStreamRecommendations;
    TSourceContext RemindersApi;
    TSourceContext ReqWizard;
    TSourceContext RouterVia;

    TSourceContext Search;
    TSourceContext SerpSummarization;
    TSourceContext SerpSummarizationAsync;
    TSourceContext SocialApi;
    TSourceContext SupProvider;

    TSourceContext TankerApi;
    TSourceContext TaxiApiProxy;
    TSourceContext TaxiSupportApi;
    TSourceContext TrafficForecastBackground;
    TSourceContext Translate;
    TSourceContext TranslateDict;
    TSourceContext TranslateMtAlice;
    TSourceContext TranslateTranslit;
    TSourceContext TranslateIsTranslit;
    TSourceContext TVGeo;
    TSourceContext Tvm2;
    TSourceContext TVSchedule;
    TSourceContext TVSearch;

    TSourceContext UGCDb;

    TSourceContext VideoAmediateka;
    TSourceContext VideoHostingSeriesEpisodes;
    TSourceContext VideoHostingTvChannels;
    TSourceContext VideoHostingTvEpisodes;
    TSourceContext VideoHostingTvEpisodesAll;
    TSourceContext VideoHostingPersonalTvChannel;
    TSourceContext VideoHostingPlayer;
    TSourceContext VideoIvi;
    TSourceContext VideoKinopoisk;
    TSourceContext VideoKinopoiskUAPI;
    TSourceContext VideoLsOtt;
    TSourceContext VideoOkkoUAPI;
    TSourceContext VideoYandexRecommendation;
    TSourceContext VideoYandexSearch;
    TSourceContext VideoYandexSearchOld;
    TSourceContext VideoYouTube;

    TSourceContext WeatherNowcast;
    TSourceContext WeatherV3;
    TSourceContext WeatherNowcastV3;

    TSourceContext XivaProvider;

    TSourceContext YandexFunctions;
    TSourceContext YaRadioAccount;
    TSourceContext YaRadioBackground;
    TSourceContext YaRadioDashboard;

    const TVector<TString>& GetRegistryList() const;

private:
    TSourceContext Register(
        const TString& source,
        NBASS::NMetrics::ICountersPlace& counters,
        TSourceContext::TConfigAccessor configAccessor);
};

/**
 * Wrapper over TSourcesRegistry for creating TSourceRequestFactory for a source with specified config
 */
class TSourcesRequestFactory {
public:
    TSourcesRequestFactory(const TSourcesRegistry& sources, const TConfig& config, const NAlice::TExpFlags& expFlags = Default<NAlice::TExpFlags>());

    TSourceRequestFactory AbuseApi() {
        return SF(Sources.AbuseApi);
    }
    TSourceRequestFactory Afisha() {
        return SF(Sources.Afisha);
    }
    TSourceRequestFactory AliceGraph() {
        return SF(Sources.AliceGraph);
    }
    TSourceRequestFactory ApiMapsStaticMapRouter() {
        return SF(Sources.ApiMapsStaticMapRouter);
    }
    TSourceRequestFactory AviaBackend() {
        return SF(Sources.AviaBackend);
    }
    TSourceRequestFactory AviaPriceIndex() {
        return SF(Sources.AviaPriceIndex);
    }
    TSourceRequestFactory AviaPriceIndexMinPrice() {
        return SF(Sources.AviaPriceIndexMinPrice);
    }
    TSourceRequestFactory AviaSuggests() {
        return SF(Sources.AviaSuggests);
    }

    TSourceRequestFactory AviaTDApiInit() {
        return SF(Sources.AviaTDApiInit);
    }
    TSourceRequestFactory BlackBox(bool is_test = false) {
        return SF(is_test ? Sources.BlackBoxTest : Sources.BlackBox);
    }
    TSourceRequestFactory CalendarApi(TStringBuf path) {
        return SF(Sources.CalendarApi, path);
    }
    TSourceRequestFactory CarRoutes() {
        return SF(Sources.CarRoutes);
    }
    TSourceRequestFactory CarsharingGreetingPhrase() {
        return SF(Sources.CarsharingGreetingPhrase);
    }
    TSourceRequestFactory CloudApiDisk() {
        return SF(Sources.CloudApiDisk);
    }
    TSourceRequestFactory CloudApiDiskUpload() {
        return SF(Sources.CloudApiDiskUpload);
    }
    TSourceRequestFactory ComputerVision() {
        return SF(Sources.ComputerVision);
    }
    TSourceRequestFactory ComputerVisionClothes() {
        return SF(Sources.ComputerVisionClothes);
    }
    TSourceRequestFactory ComputerVisionCbirFeatures() {
        return SF(Sources.ComputerVisionCbirFeatures);
    }
    TSourceRequestFactory Convert() {
        return SF(Sources.Convert);
    }
    TSourceRequestFactory EntitySearch() {
        return SF(Sources.EntitySearch);
    }
    TSourceRequestFactory ExternalSkillsApi() {
        return SF(Sources.ExternalSkillsApi);
    }
    TSourceRequestFactory ExternalSkillsDb() {
        return SF(Sources.ExternalSkillsDb);
    }
    TSourceRequestFactory ExternalSkillsKvSaaS() {
        return SF(Sources.ExternalSkillsKvSaaS);
    }
    TSourceRequestFactory ExternalSkillsRecommender() {
        return SF(Sources.ExternalSkillsRecommender);
    }
    TSourceRequestFactory ExternalSkillsSaaS() {
        return SF(Sources.ExternalSkillsSaaS);
    }
    TSourceRequestFactory GeneralConversationTurkish() {
        return SF(Sources.GeneralConversationTurkish);
    }
    TSourceRequestFactory GeoCoderLl2Geo() {
        return SF(Sources.GeoCoderLl2Geo);
    }
    TSourceRequestFactory GeoCoderTextToRoadName() {
        return SF(Sources.GeoCoderTextToRoadName);
    }
    TSourceRequestFactory GeoMetaSearchOrganization() {
        return SF(Sources.GeoMetaSearchOrganization);
    }
    TSourceRequestFactory GeoMetaSearchReverseResolve() {
        return SF(Sources.GeoMetaSearchReverseResolve);
    }
    TSourceRequestFactory GeoMetaSearchResolveText() {
        return SF(Sources.GeoMetaSearchResolveText);
    }
    TSourceRequestFactory GeoMetaSearchResolveTextNextPage() {
        return SF(Sources.GeoMetaSearchResolveTextNextPage);
    }
    TSourceRequestFactory GeoMetaSearchRoutePointContext() {
        return SF(Sources.GeoMetaSearchRoutePointContext);
    }
    TSourceRequestFactory MapsInfoExportBackground(TStringBuf path) {
        return SF(Sources.MapsInfoExportBackground, path);
    }
    TSourceRequestFactory Market() {
        return SF(Sources.Market);
    }
    TSourceRequestFactory MarketBlue() {
        return SF(Sources.MarketBlue);
    }
    TSourceRequestFactory MarketBlueHeavy() {
        return SF(Sources.MarketBlueHeavy);
    }
    TSourceRequestFactory MarketCheckouter(const TStringBuf path) {
        return SF(Sources.MarketCheckouter, path);
    }
    TSourceRequestFactory MarketCheckouterHeavy(const TStringBuf path) {
        return SF(Sources.MarketCheckouterHeavy, path);
    }
    TSourceRequestFactory MarketBeruMyBonusesList() {
        return SF(Sources.MarketBeruMyBonusesList);
    }
    TSourceRequestFactory MarketCheckouterIntervals(const TStringBuf path) {
        return SF(Sources.MarketCheckouterIntervals, path);
    }
    TSourceRequestFactory MarketCheckouterOrders(const TStringBuf path) {
        return SF(Sources.MarketCheckouterOrders, path);
    }
    TSourceRequestFactory MarketFormalizer() {
        return SF(Sources.MarketFormalizer);
    }
    TSourceRequestFactory MarketHeavy() {
        return SF(Sources.MarketHeavy);
    }
    TSourceRequestFactory MarketMds(const TStringBuf path) {
        return SF(Sources.MarketMds, path);
    }
    TSourceRequestFactory MarketStockStorage(const TStringBuf path) {
        return SF(Sources.MarketStockStorage, path);
    }
    TSourceRequestFactory MarketPersBasket(const TStringBuf path) {
        return SF(Sources.MarketPersBasket, path);
    }
    TSourceRequestFactory MarketPersBasket() {
        return SF(Sources.MarketPersBasket);
    }
    TSourceRequestFactory MassTransitRoutes() {
        return SF(Sources.MassTransitRoutes);
    }
    TSourceRequestFactory Music(TStringBuf path) {
        return SF(Sources.Music, path);
    }
    TSourceRequestFactory MusicCatalog(TStringBuf path) {
        return SF(Sources.MusicCatalog, path);
    }
    TSourceRequestFactory MusicCatalogBulk(TStringBuf path) {
        return SF(Sources.MusicCatalogBulk, path);
    }
    TSourceRequestFactory MusicQuasar(TStringBuf path) {
        return SF(Sources.MusicQuasar, path);
    }
    TSourceRequestFactory MusicSuggests(TStringBuf path) {
        return SF(Sources.MusicSuggests, path);
    }
    TSourceRequestFactory MusicAvatarsColor(TStringBuf path) {
        return SF(Sources.MusicAvatarsColor, path);
    }
    TSourceRequestFactory NerApi() {
        return SF(Sources.NerApi);
    }
    TSourceRequestFactory News() {
        return SF(Sources.News);
    }
    TSourceRequestFactory NewsApi(TStringBuf path) {
        return SF(Sources.NewsApi, path);
    }
    TSourceRequestFactory NormalizedQuery() {
        return SF(Sources.NormalizedQuery);
    }
    TSourceRequestFactory Passport() {
        return SF(Sources.Passport);
    }
    TSourceRequestFactory PedestrianRoutes() {
        return SF(Sources.PedestrianRoutes);
    }
    TSourceRequestFactory PersonalData(TStringBuf path, bool isTestBlackBox) {
        return SF(isTestBlackBox ? Sources.PersonalDataTest : Sources.PersonalData, path);
    }
    TSourceRequestFactory QuasarBillingContentBuy(TStringBuf path) {
        return SF(Sources.QuasarBillingContentBuy, path);
    }
    TSourceRequestFactory QuasarBillingContentPlay(TStringBuf path) {
        return SF(Sources.QuasarBillingContentPlay, path);
    }
    TSourceRequestFactory QuasarBillingPromoAvailability(TStringBuf path) {
        return SF(Sources.QuasarBillingPromoAvailability, path);
    }
    TSourceRequestFactory QuasarBillingSkills(TStringBuf path) {
        return SF(Sources.QuasarBillingSkills, path);
    }
    TSourceRequestFactory RadioStream(TStringBuf path) {
        return SF(Sources.RadioStream, path);
    }
    TSourceRequestFactory RadioStreamAvailableStations() {
        return SF(Sources.RadioStreamAvailableStations);
    }
    TSourceRequestFactory RadioStreamRecommendations() {
        return SF(Sources.RadioStreamRecommendations);
    }
    TSourceRequestFactory RemindersApi(TStringBuf path) {
        return SF(Sources.RemindersApi, path);
    }
    TSourceRequestFactory ReqWizard() {
        return SF(Sources.ReqWizard);
    }
    TSourceRequestFactory RouterVia() {
        return SF(Sources.RouterVia);
    }
    TSourceRequestFactory Search() {
        return SF(Sources.Search);
    }
    TSourceRequestFactory SerpSummarization() {
        return SF(Sources.SerpSummarization);
    }
    TSourceRequestFactory SerpSummarizationAsync(TStringBuf uri) {
        auto sf = SF(Sources.SerpSummarizationAsync);
        sf.OverrideUri(uri);
        return sf;
    }
    TSourceRequestFactory SocialApi(TStringBuf path = TStringBuf("/api/token/newest")) {
        return SF(Sources.SocialApi, path);
    }
    TSourceRequestFactory SupProvider() {
        return SF(Sources.SupProvider);
    }
    TSourceRequestFactory TankerApi() {
        return SF(Sources.TankerApi);
    }
    TSourceRequestFactory TaxiApi(TStringBuf path, bool isSupport) {
        if (isSupport)
            return SF(Sources.TaxiSupportApi, path);
        return SF(Sources.TaxiApiProxy, path);
    }
    TSourceRequestFactory TrafficForecastBackground(TStringBuf path) {
        return SF(Sources.TrafficForecastBackground, path);
    }
    TSourceRequestFactory Translate() {
        return SF(Sources.Translate);
    }
    TSourceRequestFactory TranslateDict() {
        return SF(Sources.TranslateDict);
    }
    TSourceRequestFactory TranslateMtAlice(TStringBuf path) {
        return SF(Sources.TranslateMtAlice, path);
    }
    TSourceRequestFactory TranslateTranslit() {
        return SF(Sources.TranslateTranslit);
    }
    TSourceRequestFactory TranslateIsTranslit() {
        return SF(Sources.TranslateIsTranslit);
    }
    TSourceRequestFactory TVGeo(TStringBuf path) {
        return SF(Sources.TVGeo, path);
    }
    TSourceRequestFactory TVSearch(TStringBuf path) {
        return SF(Sources.TVSearch, path);
    }
    TSourceRequestFactory TVSchedule(TStringBuf path) {
        return SF(Sources.TVSchedule, path);
    }
    TSourceRequestFactory UGCDb(TStringBuf path) {
        return SF(Sources.UGCDb, path);
    }
    TSourceRequestFactory VideoAmediateka(TStringBuf path) {
        return SF(Sources.VideoAmediateka, path);
    }
    TSourceRequestFactory VideoHostingSeriesEpisodes() {
        return SF(Sources.VideoHostingSeriesEpisodes);
    }
    TSourceRequestFactory VideoHostingTvChannels() {
        return SF(Sources.VideoHostingTvChannels);
    }
    TSourceRequestFactory VideoHostingTvEpisodes() {
        return SF(Sources.VideoHostingTvEpisodes);
    }
    TSourceRequestFactory VideoHostingTvEpisodesAll() {
        return SF(Sources.VideoHostingTvEpisodesAll);
    }
    TSourceRequestFactory VideoHostingPersonalTvChannel() {
        return SF(Sources.VideoHostingPersonalTvChannel);
    }
    TSourceRequestFactory VideoHostingPlayer(TStringBuf path) {
        return SF(Sources.VideoHostingPlayer, path);
    }
    TSourceRequestFactory VideoIvi(TStringBuf path) {
        return SF(Sources.VideoIvi, path);
    }
    TSourceRequestFactory VideoKinopoisk(TStringBuf path) {
        return SF(Sources.VideoKinopoisk, path);
    }
    TSourceRequestFactory VideoKinopoiskUAPI(TStringBuf path) {
        return SF(Sources.VideoKinopoiskUAPI, path);
    }
    TSourceRequestFactory VideoLsOtt(TStringBuf path) {
        return SF(Sources.VideoLsOtt, path);
    }
    TSourceRequestFactory VideoOkkoUAPI(TStringBuf path) {
        return SF(Sources.VideoOkkoUAPI, path);
    }
    TSourceRequestFactory VideoYandexRecommendation() {
        return SF(Sources.VideoYandexRecommendation);
    }
    TSourceRequestFactory VideoYandexSearch() {
        return SF(Sources.VideoYandexSearch);
    }
    TSourceRequestFactory VideoYandexSearchOld() {
        return SF(Sources.VideoYandexSearchOld);
    }
    TSourceRequestFactory VideoYouTube(TStringBuf path) {
        return SF(Sources.VideoYouTube, path);
    }
    TSourceRequestFactory WeatherNowcast() {
        return SF(Sources.WeatherNowcast);
    }
    TSourceRequestFactory WeatherV3() {
        return SF(Sources.WeatherV3);
    }
    TSourceRequestFactory WeatherNowcastV3() {
        return SF(Sources.WeatherNowcastV3);
    }
    TSourceRequestFactory XivaProvider() {
        return SF(Sources.XivaProvider);
    }
    TSourceRequestFactory YandexFunctions(TStringBuf path) {
        return SF(Sources.YandexFunctions, path);
    }
    TSourceRequestFactory YaRadioAccount(TStringBuf path) {
        return SF(Sources.YaRadioAccount, path);
    }
    TSourceRequestFactory YaRadioBackground(TStringBuf path) {
        return SF(Sources.YaRadioBackground, path);
    }
    TSourceRequestFactory YaRadioDashboard(TStringBuf path) {
        return SF(Sources.YaRadioDashboard, path);
    }

private:
    TSourceRequestFactory SF(const TSourceContext& source, TMaybe<TStringBuf> appendPath = Nothing()) {
        return TSourceRequestFactory(source, Config, appendPath, Sources.SourcesRegistryDelegate, ExpFlags);
    }

private:
    const TSourcesRegistry& Sources;
    const TConfig& Config;
    const NAlice::TExpFlags& ExpFlags;
};

void ProxyRequestViaZora(const TConfig& config, NHttpFetcher::TRequest* request, const ISourcesRegistryDelegate& sourcesRegistryDelegate, bool enableSslCheck = false);
