#include "source_request.h"

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>

#include <alice/library/network/headers.h>

#include <library/cpp/cgiparam/cgiparam.h>

#include <util/string/builder.h>

namespace {

using TCustomErrorCallback = std::function<void(const NHttpFetcher::TResponse& response)>;

class TCustomStatusCallback : public NHttpFetcher::IStatusCallback {
public:
    TCustomStatusCallback(NHttpFetcher::IStatusCallback::TRef slave, TCustomResponseCallback customCallback,
                          TCustomErrorCallback errorCallback)
        : Slave(slave)
        , CustomCallback(customCallback)
        , ErrorCallback(errorCallback)
    {
    }

    NHttpFetcher::TFetchStatus OnResponse(const NHttpFetcher::TResponse& response) override {
        const NHttpFetcher::TFetchStatus status = Slave->OnResponse(response);
        if (!status.IsSuccess()) {
            ErrorCallback(response);
            return status;
        }
        return CustomCallback(response.Data);
    }

private:
    NHttpFetcher::IStatusCallback::TRef Slave;
    TCustomResponseCallback CustomCallback;
    TCustomErrorCallback ErrorCallback;
};

class TMetrics : public NHttpFetcher::IMetrics {
public:
    TMetrics(TStringBuf prefix, NBASS::NMetrics::ICountersPlace& counters)
        : Prefix(prefix)
        , ResponseTimeHgram(TString::Join(Prefix, TStringBuf("_responseTime")))
        , ResponseTimeSLAHgram(TString::Join(ResponseTimeHgram, TStringBuf("_SLA")))
    {
        counters.BassCounters().RegisterUnistatHistogram(ResponseTimeHgram);
        counters.BassCounters().RegisterUnistatHistogram(ResponseTimeSLAHgram);
    }

    void OnStartRequest() override {
        IncMetric(TStringBuf("requests_total"));
    }
    void OnFinishRequest(const NHttpFetcher::TResponseStats& stats) override {
        TrackResponseTime(stats.Duration, stats.SlaTime);
        const auto& status = stats.Status;
        if (status.IsSuccess()) {
            IncMetric(TStringBuf("requests_success"));
        } else {
            IncMetric(TStringBuf("requests_failure"), status.Reason);
            IncMetric(TStringBuf("requests_failure"));
        }
    }

    void OnStartHedgedRequest(const NHttpFetcher::TRequestStats& /* stats */) override {
        IncMetric(TStringBuf("hedgedRequests_total"));
    }
    void OnFinishHedgedRequest(const NHttpFetcher::TResponseStats& stats) override {
        const auto& status = stats.Status;
        if (status.IsSuccess()) {
            IncMetric(TStringBuf("hedgedRequests_success"));
        } else {
            IncMetric(TStringBuf("requests_error"), status.Reason);
        }
    }

private:
    void IncMetric(TStringBuf name, TStringBuf suffix = TStringBuf()) {
        TStringBuilder sensor;
        sensor << Prefix << '_' << name;

        if (suffix) {
            sensor << '_' << suffix;
        }
        Y_STATS_INC_COUNTER(sensor);
    }
    void TrackResponseTime(TDuration d, TDuration slaTime) {
        NMonitoring::GetHistogram(ResponseTimeHgram).Record(d.MilliSeconds());
        TUnistat::Instance().PushSignalUnsafe(ResponseTimeHgram, d.MilliSeconds());

        ui64 overSLA = Max<ui64>(0, (d - slaTime).MilliSeconds());
        NMonitoring::GetHistogram(ResponseTimeSLAHgram).Record(overSLA);
        TUnistat::Instance().PushSignalUnsafe(ResponseTimeSLAHgram, overSLA);
    }

private:
    const TString Prefix;
    const TString ResponseTimeHgram;
    const TString ResponseTimeSLAHgram;
};

using TSourceConst = NBASSConfig::TConfigConst<TSchemeTraits>::TSourceConst;

} // (anonymous namespace)



TSourceContext::TSourceContext(const TString& source, const TString& requestType, NBASS::NMetrics::ICountersPlace& counters, TConfigAccessor configAccessor)
    : ConfigAccessor(configAccessor)
    , Metrics(new TMetrics(TStringBuilder() << "source_" << source << "_type_" << requestType, counters))
    , Source(source)
    , RequestType(requestType)
{
}

TSourceContext::TSourceContext(const TString& source, NBASS::NMetrics::ICountersPlace& counters, TConfigAccessor configAccessor)
    : ConfigAccessor(configAccessor)
    , Metrics(new TMetrics(TStringBuilder() << "source_" << source, counters))
    , Source(source)
{
}

TSourceContext::TSourceContext(TSourceContext&& sourceContext)
    : ConfigAccessor(std::move(sourceContext.ConfigAccessor))
    , Metrics(std::move(sourceContext.Metrics))
    , Source(std::move(sourceContext.Source))
    , RequestType(std::move(sourceContext.RequestType))
{
}

TSourceRequestFactory::TSourceRequestFactory(const TSourceContext& context, const TConfig& config, TMaybe<TStringBuf> appendPath, const ISourcesRegistryDelegate& sourcesRegistryDelegate, const NAlice::TExpFlags& expFlags)
    : Context(context)
    , Config(config)
    , AppendPath(appendPath)
    , SourcesRegistryDelegate(sourcesRegistryDelegate)
    , ExpFlags(expFlags)
{
}

namespace {

NHttpFetcher::TRequestOptions SourceRequestOptions(
    const TSourceContext& context,
    const TConfig& config,
    TCustomResponseCallback customCallback)
{
    TSourceConst source = context.ConfigAccessor(config);

    auto onError = [&context](const NHttpFetcher::TResponse& response) {
        LOG(ERR) << context.Source << " request failed with status code "
            << response.Code << ": " << response.GetErrorText() << Endl;
    };

    NHttpFetcher::TRequestOptions options {
        .RetryPeriod = source.RetryPeriod(),
        .Timeout = config.OverrideAllTimeouts() == TDuration::Zero() ? source.Timeout() : config.OverrideAllTimeouts(),
        .MaxAttempts = config.AliceFetcherNoRetry() ? static_cast<ui32>(1) : source.MaxAttempts(),
        .EnableFastReconnect = source.EnableFastReconnect(),
        .Metrics = context.Metrics,
        .SLATime = source.SLATime(),
        .StatusCallback = new TCustomStatusCallback(NHttpFetcher::DefaultStatusCallback(), customCallback, onError),
        .ProxyOverride = config.GetProxyOverride(),
        .LogErrors = true,
    };

    options.BeforeFetchCallback = [&context](const NHttpFetcher::TRequest& request) {
        TStringBuilder b;
        b << context.Source;
        if (context.RequestType) {
            b << ' ' << *context.RequestType;
        }
        // FIXME logging proxy
        LOG(DEBUG) << b << TStringBuf(" request: ") << request.Url() << Endl;
    };

    return options;
}

} // namespace anonymous

NHttpFetcher::TRequestPtr TSourceRequestFactory::Request() const {
    return Request([] (const TString&) { return NHttpFetcher::TFetchStatus::Success(); });
}

NHttpFetcher::TRequestPtr TSourceRequestFactory::Request(TCustomResponseCallback customCallback) const {
    NHttpFetcher::TRequestPtr request = NHttpFetcher::Request(RequestUrl(), SourceRequestOptions(Context, Config, customCallback));
    FillAdditionalFields(request.Get());
    return request;
}

NHttpFetcher::TRequestPtr TSourceRequestFactory::AttachRequest(NHttpFetcher::IMultiRequest::TRef multiRequest) const {
    return AttachRequest(multiRequest, [] (const TString&) { return NHttpFetcher::TFetchStatus::Success(); });
}

NHttpFetcher::TRequestPtr TSourceRequestFactory::AttachRequest(NHttpFetcher::IMultiRequest::TRef multiRequest, TCustomResponseCallback customCallback) const {
    NHttpFetcher::TRequestPtr request = multiRequest->AddRequest(RequestUrl(), SourceRequestOptions(Context, Config, customCallback));
    FillAdditionalFields(request.Get());
    return request;
}

NHttpFetcher::TRequestPtr TSourceRequestFactory::MakeOrAttachRequest(NHttpFetcher::IMultiRequest::TRef multiRequest) {
    return multiRequest ? AttachRequest(multiRequest) : Request();
}

NHttpFetcher::TRequestPtr TSourceRequestFactory::MakeOrAttachRequest(NHttpFetcher::IMultiRequest::TRef multiRequest, TCustomResponseCallback callback) {
    return multiRequest ? AttachRequest(multiRequest, callback) : Request(callback);
}

NUri::TUri TSourceRequestFactory::RequestUrl() const {
    TSourceConst source = Context.ConfigAccessor(Config);

    TStringBuf uriStr = source.Host();
    if (UriOverride.Defined()) {
        uriStr = *UriOverride;
    }

    NUri::TUri uri = NHttpFetcher::ParseUri(uriStr);
    if (AppendPath.Defined()) {
        uri = NAlice::NNetwork::AppendToUri(uri, *AppendPath);
    }
    return uri;
}

void TSourceRequestFactory::FillAdditionalFields(NHttpFetcher::TRequest* request) const {
    TSourceConst source = Context.ConfigAccessor(Config);
    if (source.External()) {
        ProxyRequestViaZora(Config, request, SourcesRegistryDelegate);
    } else if (!source.Tvm2ClientId()->empty()) {
        TMaybe<TString> serviceTicket = SourcesRegistryDelegate.GetTVM2ServiceTicket(source.Tvm2ClientId());
        if (serviceTicket) {
            request->AddHeader("X-Ya-Service-Ticket", *serviceTicket);
        }
    }

    request->SetRequestLabel(Context.Source);

    const auto crossDcExperimentName = TStringBuilder() << NBASS::EXPERIMANTAL_FLAG_ENABLE_SOURCE_CROSS_DC_PREFIX << Context.Source;
    if (const auto* currentDc = Config.GetCurrentDC(); currentDc && !ExpFlags.Has(crossDcExperimentName)) {
        request->AddHeader(NAlice::NNetwork::HEADER_X_BALANCER_DC_HINT, *currentDc);
        request->AddHeader(NAlice::NNetwork::HEADER_X_ALICE_INTERNAL_REQUEST, "yes");
    }

    TString addCgiExp = TStringBuilder() << "bass_add_cgi_to_source_" << Context.Source;
    if (const auto& maybeCgi = ExpFlags.Value(addCgiExp); maybeCgi.Defined()) {
        TCgiParameters cgi(*maybeCgi);
        if (!cgi.empty()) {
            request->AddCgiParams(cgi);
        }
    }
}

TSourcesRegistry::TSourcesRegistry(NBASS::NMetrics::ICountersPlace& counters, const ISourcesRegistryDelegate& sourcesRegistryDelegate)
    : SourcesRegistryDelegate(sourcesRegistryDelegate)
    , AbuseApi(Register("AbuseApi", counters, [] (const TConfig& c) { return c.Vins().AbuseApi(); }))
    , Afisha(Register("Afisha", counters, [] (const TConfig& c) { return c.Vins().Afisha(); }))
    , AliceGraph(Register("AliceGraph", counters, [] (const TConfig& c) {return c.Vins().AliceGraph(); }))
    , ApiMapsStaticMapRouter(Register("ApiMapsStaticMapRouter", counters, [] (const TConfig& c) { return c.Vins().ApiMapsStaticMapRouter(); }))
    , AviaBackend(Register("AviaBackend", counters, [] (const TConfig& c) { return c.Vins().AviaBackend(); }))
    , AviaPriceIndex(Register("AviaPriceIndex", counters, [] (const TConfig& c) {return c.Vins().AviaPriceIndex(); }))
    , AviaPriceIndexMinPrice(Register("AviaPriceIndexMinPrice", counters, [] (const TConfig& c) {return c.Vins().AviaPriceIndexMinPrice(); }))
    , AviaSuggests(Register("AviaSuggests", counters, [] (const TConfig& c) {return c.Vins().AviaSuggests(); }))
    , AviaTDApiInit(Register("AviaTDApiInit", counters, [] (const TConfig& c) {return c.Vins().AviaTDApiInit(); }))
    , BlackBox(Register("BlackBox", counters, [] (const TConfig& c) {return c.Vins().BlackBox(); }))
    , BlackBoxTest(Register("BlackBoxTest", counters, [] (const TConfig& c) {return c.Vins().BlackBoxTest(); }))
    , CalendarApi(Register("CalendarApi", counters, [] (const TConfig& c) { return c.Vins().CalendarApi(); }))
    , CarRoutes(Register("CarRoutes", counters, [] (const TConfig& c) { return c.Vins().CarRoutes(); }))
    , CarsharingGreetingPhrase(Register("CarsharingGreetingPhrase", counters, [] (const TConfig& c) { return c.Vins().CarsharingGreetingPhrase(); }))
    , CloudApiDisk(Register("CloudApiDisk", counters, [] (const TConfig& c) { return c.Vins().CloudApiDisk(); }))
    , CloudApiDiskUpload(Register("CloudApiDiskUpload", counters, [] (const TConfig& c) { return c.Vins().CloudApiDiskUpload(); }))
    , ComputerVision(Register("ComputerVision", counters, [] (const TConfig& c) { return c.Vins().ComputerVision(); }))
    , ComputerVisionClothes(Register("ComputerVisionClothes", counters, [] (const TConfig& c) { return c.Vins().ComputerVisionClothes(); }))
    , ComputerVisionCbirFeatures(Register("ComputerVisionCbirFeatures", counters, [] (const TConfig& c) { return c.Vins().ComputerVisionCbirFeatures(); }))
    , Convert(Register("UnitsConverter", counters, [] (const TConfig& c) { return c.Vins().UnitsConverter(); }))
    , EntitySearch(Register("EntitySearch", counters, [] (const TConfig& c) { return c.Vins().EntitySearch(); }))
    , ExternalSkillsApi(Register("ExternalSkillsApi", counters, [] (const TConfig& c) { return c.Vins().ExternalSkillsApi(); }))
    , ExternalSkillsDb(Register("ExternalSkillsDb", counters, [] (const TConfig& c) { return c.Vins().ExternalSkillsDb(); }))
    , ExternalSkillsKvSaaS(Register("ExternalSkillsKvSaaS", counters, [] (const TConfig& c) { return c.Vins().ExternalSkillsKvSaaS(); }))
    , ExternalSkillsRecommender(Register("ExternalSkillsRecommender", counters, [] (const TConfig& c) { return c.Vins().ExternalSkillsRecommender(); }))
    , ExternalSkillsSaaS(Register("ExternalSkillsSaaS", counters, [] (const TConfig& c) { return c.Vins().ExternalSkillsSaaS(); }))
    , GeneralConversationTurkish(Register("GeneralConversationTurkish", counters, [] (const TConfig& c) { return c.Vins().GeneralConversationTurkish(); }))
    , GeoCoderLl2Geo(Register("GeoCoderLL2Geo", counters, [] (const TConfig& c) { return c.Vins().GeoCoderLL2Geo(); }))
    , GeoCoderTextToRoadName(Register("GeoCoderText2RoadName", counters, [] (const TConfig& c) { return c.Vins().GeoCoderText2RoadName(); }))
    , GeoMetaSearchOrganization(Register("GeoMetaSearchOrganization", counters, [] (const TConfig& c) { return c.Vins().GeoMetaSearchOrganization(); }))
    , GeoMetaSearchResolveText(Register("GeoMetaSearchResolveText", counters, [] (const TConfig& c) { return c.Vins().GeoMetaSearchResolveText(); }))
    , GeoMetaSearchResolveTextNextPage(Register("GeoMetaSearchResolveTextNextPage", counters, [] (const TConfig& c) { return c.Vins().GeoMetaSearchResolveTextNextPage(); }))
    , GeoMetaSearchReverseResolve(Register("GeoMetaSearchReverseResolve", counters, [] (const TConfig& c) { return c.Vins().GeoMetaSearchReverseResolve(); }))
    , GeoMetaSearchRoutePointContext(Register("GeoMetaSearchRoutePointContext", counters, [] (const TConfig& c) { return c.Vins().GeoMetaSearchRoutePointContext(); }))
    , MapsInfoExportBackground(Register("MapsInfoExportBackground", counters, [] (const TConfig& c) { return c.Vins().MapsInfoExportBackground(); }))
    , Market(Register("Market", counters, [] (const TConfig& c) { return c.Vins().Market(); }))
    , MarketBlue(Register("MarketBlue", counters, [] (const TConfig& c) { return c.Vins().MarketBlue(); }))
    , MarketBlueHeavy(Register("MarketBlueHeavy", counters, [] (const TConfig& c) { return c.Vins().MarketBlueHeavy(); }))
    , MarketCheckouter(Register("MarketCheckouter", counters, [] (const TConfig& c) { return c.Vins().MarketCheckouter(); }))
    , MarketCheckouterHeavy(Register("MarketCheckouterHeavy", counters, [] (const TConfig& c) { return c.Vins().MarketCheckouterHeavy(); }))
    , MarketCheckouterIntervals(Register("MarketCheckouterIntervals", counters, [] (const TConfig& c) { return c.Vins().MarketCheckouterIntervals(); }))
    , MarketCheckouterOrders(Register("MarketCheckouterOrders", counters, [] (const TConfig& c) { return c.Vins().MarketCheckouterOrders(); }))
    , MarketFormalizer(Register("MarketFormalizer", counters, [] (const TConfig& c) { return c.Vins().MarketFormalizer(); }))
    , MarketHeavy(Register("MarketHeavy", counters, [] (const TConfig& c) { return c.Vins().MarketHeavy(); }))
    , MarketMds(Register("MarketMds", counters, [] (const TConfig& c) { return c.Vins().MarketMds(); }))
    , MarketStockStorage(Register("MarketStockStorage", counters, [] (const TConfig& c) { return c.Vins().MarketStockStorage(); }))
    , MarketPersBasket(Register("MarketPersBasket", counters, [] (const TConfig& c) { return c.Vins().MarketPersBasket(); }))
    , MarketBeruMyBonusesList(Register("MarketBeruMyBonusesList", counters, [] (const TConfig& c) { return c.Vins().MarketBeruMyBonusesList(); }))
    , MassTransitRoutes(Register("MassTransitRoutes", counters, [] (const TConfig& c) { return c.Vins().MassTransitRoutes(); }))
    , Music(Register("Music", counters, [] (const TConfig& c) { return c.Vins().Music(); }))
    , MusicCatalog(Register("MusicCatalog", counters, [] (const TConfig& c) { return c.Vins().MusicCatalog(); }))
    , MusicCatalogBulk(
          Register("MusicCatalogBulk", counters, [](const TConfig& c) { return c.Vins().MusicCatalogBulk(); }))
    , MusicQuasar(Register("MusicQuasar", counters, [] (const TConfig& c) { return c.Vins().MusicQuasar(); }))
    , MusicSuggests(Register("MusicSuggests", counters, [] (const TConfig& c) { return c.Vins().MusicSuggests(); }))
    , MusicAvatarsColor(Register("MusicAvatarsColor", counters, [] (const TConfig& c) { return c.Vins().MusicAvatarsColor(); }))
    , NerApi(Register("NerApi", counters, [] (const TConfig& c) { return c.Vins().NerApi(); }))
    , News(Register("News", counters, [] (const TConfig& c) { return c.Vins().News(); }))
    , NewsApi(Register("NewsApi", counters, [] (const TConfig& c) { return c.Vins().NewsApi(); }))
    , NewsApiScheduler(Register("NewsApiScheduler", counters, [] (const TConfig& c) { return c.Vins().NewsApiScheduler(); }))
    , NormalizedQuery(Register("NormalizedQuery", counters, [] (const TConfig& c) { return c.Vins().NormalizedQuery(); }))
    , Passport(Register("Passport", counters, [] (const TConfig& c) { return c.Vins().Passport(); }))
    , PedestrianRoutes(Register("PedestrianRoutes", counters, [] (const TConfig& c) { return c.Vins().PedestrianRoutes(); }))
    , PersonalData(Register("PersonalData", counters, [] (const TConfig& c) { return c.Vins().PersonalData(); }))
    , PersonalDataTest(Register("PersonalData", counters, [] (const TConfig& c) { return c.Vins().PersonalDataTest(); }))
    , QuasarBillingContentBuy(Register("QuasarBillingContentBuy", counters,
                                          [](const TConfig& c) { return c.Vins().QuasarBillingContentBuy(); }))
    , QuasarBillingContentPlay(Register("QuasarBillingContentPlay", counters,
                                           [](const TConfig& c) { return c.Vins().QuasarBillingContentPlay(); }))
    , QuasarBillingPromoAvailability(
          Register("QuasarBillingPromoAvailability", counters,
                   [](const TConfig& c) { return c.Vins().QuasarBillingPromoAvailability(); }))
    , QuasarBillingSkills(Register("QuasarBillingSkills", counters,
                                      [](const TConfig& c) { return c.Vins().QuasarBillingSkills(); }))
    , RadioStream(Register("RadioStream", counters, [] (const TConfig& c) { return c.Vins().RadioStream(); }))
    , RadioStreamAvailableStations(Register("RadioStreamAvailableStations", counters, [] (const TConfig& c) { return c.Vins().RadioStreamAvailableStations(); }))
    , RadioStreamRecommendations(Register("RadioStreamRecommendations", counters, [] (const TConfig& c) { return c.Vins().RadioStreamRecommendations(); }))
    , RemindersApi(Register("RemindersApi", counters, [] (const TConfig& c) { return c.Vins().RemindersApi(); }))
    , ReqWizard(Register("ReqWizard", counters, [] (const TConfig& c) { return c.Vins().ReqWizard(); }))
    , RouterVia(Register("RouterVia", counters, [] (const TConfig& c) { return c.Vins().RouterVia(); }))
    , Search(Register("Search", counters, [] (const TConfig& c) { return c.Vins().Search(); }))
    , SerpSummarization(Register("SerpSummarization", counters, [] (const TConfig& c) { return c.Vins().SerpSummarization(); }))
    , SerpSummarizationAsync(Register("SerpSummarizationAsync", counters, [] (const TConfig& c) { return c.Vins().SerpSummarizationAsync(); }))
    , SocialApi(Register("SocialApi", counters, [] (const TConfig& c) { return c.Vins().SocialApi(); }))
    , SupProvider(Register("SupProvider", counters, [] (const TConfig& c) { return c.PushHandler().SupProvider().Source(); }))
    , TankerApi(Register("TankerApi", counters, [] (const TConfig& c) { return c.Vins().TankerApi(); }))
    , TaxiApiProxy(Register("TaxiApiProxy", counters, [] (const TConfig& c) { return c.Vins().TaxiApiProxy(); }))
    , TaxiSupportApi(Register("TaxiSupportApi", counters, [] (const TConfig& c) { return c.Vins().TaxiSupportApi(); }))
    , TrafficForecastBackground(Register("TrafficForecastBackground", counters, [] (const TConfig& c) { return c.Vins().TrafficForecastBackground(); }))
    , Translate(Register("Translate", counters, [] (const TConfig& c) { return c.Vins().Translate(); }))
    , TranslateDict(Register("TranslateDict", counters, [] (const TConfig& c) { return c.Vins().TranslateDict(); }))
    , TranslateMtAlice(Register("TranslateMtAlice", counters, [] (const TConfig& c) { return c.Vins().TranslateMtAlice(); }))
    , TranslateTranslit(Register("TranslateTranslit", counters, [] (const TConfig& c) { return c.Vins().TranslateTranslit(); }))
    , TranslateIsTranslit(Register("TranslateIsTranslit", counters, [] (const TConfig& c) { return c.Vins().TranslateIsTranslit(); }))
    , TVGeo(Register("TVGeo", counters, [] (const TConfig& c) { return c.Vins().TVGeo(); }))
    , Tvm2(Register("Tvm2", counters, [] (const TConfig& c) { return c.Vins().Tvm2(); }))
    , TVSchedule(Register("TVSchedule", counters, [] (const TConfig& c) { return c.Vins().TVSchedule(); }))
    , TVSearch(Register("TVSearch", counters, [] (const TConfig& c) { return c.Vins().TVSearch(); }))
    , UGCDb(Register("UGCDb", counters, [] (const TConfig& c) { return c.Vins().UGCDb(); }))
    , VideoAmediateka(Register("VideoAmediateka", counters, [] (const TConfig& c) { return c.Vins().VideoAmediateka(); }))
    , VideoHostingSeriesEpisodes(Register("VideoHostingSeriesEpisodes", counters, [] (const TConfig& c) {return c.Vins().VideoHostingSeriesEpisodes(); }))
    , VideoHostingTvChannels(Register("VideoHostingTvChannels", counters, [] (const TConfig& c) {return c.Vins().VideoHostingTvChannels(); }))
    , VideoHostingTvEpisodes(Register("VideoHostingTvEpisodes", counters, [] (const TConfig& c) {return c.Vins().VideoHostingTvEpisodes(); }))
    , VideoHostingTvEpisodesAll(Register("VideoHostingTvEpisodesAll", counters, [] (const TConfig& c) {return c.Vins().VideoHostingTvEpisodesAll(); }))
    , VideoHostingPersonalTvChannel(Register("VideoHostingPersonalTvChannel", counters, [] (const TConfig& c) {return c.Vins().VideoHostingPersonalTvChannel(); }))
    , VideoHostingPlayer(Register("VideoHostingPlayer", counters, [] (const TConfig& c) { return c.Vins().VideoHostingPlayer(); }))
    , VideoIvi(Register("VideoIvi", counters, [] (const TConfig& c) { return c.Vins().VideoIvi(); }))
    , VideoKinopoisk(Register("VideoKinopoisk", counters, [] (const TConfig& c) { return c.Vins().VideoKinopoisk(); }))
    , VideoKinopoiskUAPI(Register("VideoKinopoiskUAPI", counters, [] (const TConfig& c) { return c.Vins().VideoKinopoiskUAPI(); }))
    , VideoLsOtt(Register("VideoLsOtt", counters, [] (const TConfig& c) { return c.Vins().VideoLsOtt(); }))
    , VideoOkkoUAPI(Register("VideoOkkoUAPI", counters, [] (const TConfig& c) { return c.Vins().VideoOkkoUAPI(); }))
    , VideoYandexRecommendation(Register("VideoYandexRecommendation", counters, [] (const TConfig& c) { return c.Vins().VideoYandexRecommendation(); }))
    , VideoYandexSearch(Register("VideoYandexSearch", counters, [] (const TConfig& c) { return c.Vins().VideoYandexSearch(); }))
    , VideoYandexSearchOld(Register("VideoYandexSearchOld", counters, [] (const TConfig& c) { return c.Vins().VideoYandexSearchOld(); }))
    , VideoYouTube(Register("VideoYouTube", counters, [] (const TConfig& c) {return c.Vins().VideoYouTube(); }))
    , WeatherNowcast(Register("WeatherNowcast", counters, [] (const TConfig& c) { return c.Vins().WeatherNowcast(); }))
    , WeatherV3(Register("WeatherV3", counters, [] (const TConfig& c) { return c.Vins().WeatherV3(); }))
    , WeatherNowcastV3(Register("WeatherNowcastV3", counters, [] (const TConfig& c) { return c.Vins().WeatherNowcastV3(); }))
    , XivaProvider(Register("XivaProvider", counters, [] (const TConfig& c) { return c.PushHandler().XivaProvider().Source(); }))
    , YandexFunctions(Register("YandexFunctions", counters, [] (const TConfig& c) { return c.Vins().YandexFunctions(); }))
    , YaRadioAccount(Register("YaRadioAccount", counters, [] (const TConfig& c) { return c.Vins().YaRadioAccount(); }))
    , YaRadioBackground(Register("YaRadioBackground", counters, [] (const TConfig& c) { return c.Vins().YaRadioBackground(); }))
    , YaRadioDashboard(Register("YaRadioDashboard", counters, [] (const TConfig& c) { return c.Vins().YaRadioDashboard(); }))
{
}

const TVector<TString>& TSourcesRegistry::GetRegistryList() const {
    return Registry;
}

TSourceContext TSourcesRegistry::Register(
    const TString& source,
    NBASS::NMetrics::ICountersPlace& counters,
    TSourceContext::TConfigAccessor configAccessor)
{
    TSourceContext sourceContext(source, counters, configAccessor);
    Registry.push_back(source);
    return sourceContext;
}

TSourcesRequestFactory::TSourcesRequestFactory(const TSourcesRegistry& sources, const TConfig& config, const NAlice::TExpFlags& expFlags)
    : Sources(sources)
    , Config(config)
    , ExpFlags(expFlags)
{
}

void ProxyRequestViaZora(const TConfig& config, NHttpFetcher::TRequest* request, const ISourcesRegistryDelegate& sourcesRegistryDelegate, bool enableSslCheck) {
    const auto& zoraProxyConfig = config.Vins().ZoraProxy();
    request->SetProxy(TString{*zoraProxyConfig.Host()});
    request->AddHeader("X-Yandex-Requesttype", "userproxy");
    request->AddHeader("X-Yandex-Sourcename", zoraProxyConfig.SourceName());
    if (TMaybe<TString> serviceTicket = sourcesRegistryDelegate.GetTVM2ServiceTicket(zoraProxyConfig.Tvm2ClientId())) {
        request->AddHeader("X-Ya-Service-Ticket", *serviceTicket);
    }
    request->AddHeader("X-Yandex-NoCalc", "1");

    if (request->Url().StartsWith(TStringBuf("https:"))) {
        request->AddHeader("X-Yandex-Use-Https", "1");
        if (enableSslCheck) {
            request->AddHeader("X-Yandex-SslCertPolicy", "Verify");
        }
    }
}
