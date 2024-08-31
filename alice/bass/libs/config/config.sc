namespace NBASSConfig;

struct TYdbConfig {
    DataBase : string (required);
    Endpoint : string (required);
    Token : string;
};

struct TRTLogConfig {
    FileName: string (default = "/dev/null");
    ServiceName: string (default = "bass");
    Async: bool (default = true);
    FlushPeriod: duration (default = "1s");
    FileStatCheckPeriod: duration (default = "1s");
};

struct TConfig {
    struct TSource {
        EnableFastReconnect   : bool (default = false);
        External              : bool (default = false);
        Host (required)       : string;
        MaxAttempts           : ui16 (default = 1);
        RetryPeriod           : duration (default = "50ms");
        Timeout               : duration (default = "150ms");
        SLATime               : duration (default = "3s");
        Tvm2ClientId          : string;
        Owner                 : string;
        Comment               : string;

        (validate) {
            if (EnableFastReconnect() == true && MaxAttempts() != 1) {
                AddError("EnableFastReconnect only works when MaxAttempts=1");
            }
        };
    };

    struct THeader {
        Name (required) : string;
        Value (required) : string;
    };

    struct TTvm2Source : TSource {
        BassTvm2ClientId : string (required);
        UpdatePeriod : duration (default = "1h");
    };

    struct TVins {
        struct TWeatherNowcastSource : TSource {
             StaticApiKey : string (required);
        };
        struct TExternalSkill : TSource {
            SkillTimeout : duration (default = "500ms");
            ZoraRequestTimeout : duration (required);
            AvatarsHost  : string (default = "avatars.mds.yandex.net");
        };
        struct TExternalSkillsRecommender : TSource {
            OnboardingUrl : string (default = "https://alice.yandex.ru/help");
            OnboardingGamesUrl : string (default = "https://alice.yandex.ru/help#games");
            StoreOnboardingUrl : string (default = "https://dialogs.yandex.ru/store/essentials?utm_source=Yandex_Alisa&utm_medium=onboarding");
            StoreOnboardingGamesUrl : string (default = "https://dialogs.yandex.ru/store/essentials#games?utm_source=Yandex_Alisa&utm_medium=onboarding");
        };
        struct TGeneralConversaion : TSource {
            MaxResults : ui16 (default = 10);
            MinRatioWithBestResponse : double (default = 1.0 );
            SearchFor : string (default = "context_and_reply");
            SearchBy : string (default = "context");
            ContextWeight : double (default = 1.0);
            RankerModelName : string (default = "catboost");
            DssmModelName : string (default = "insight_c3_rus_lister");
        };
        struct TPassportSource : TSource {
            Consumer : string;
        };
        struct TKinopoiskSource : TSource {
            ClientId : string (required);
        };

        struct TWebSearchSource : TSource {
            WaitAll : bool;
        };

        struct TSerpSummarizationSource : TSource {
            CM2TouchThreshold : double (default = 0.05);
        };

        struct TZoraProxy : TSource {
            SourceName : string (required);
        };

        AbuseApi : TSource;
        Afisha (required) : TSource;
        AliceGraph : TSource;
        ApiMapsStaticMapRouter : TSource;
        BlackBox (required) : TSource;
        BlackBoxTest (required) : TSource;
        EntitySearch (required) : TSource;
        WeatherNowcast (required) : TWeatherNowcastSource;
        WeatherV3 (required) : TSource;
        WeatherNowcastV3 (required) : TSource;
        TVGeo (required) : TSource;
        TVSearch (required) : TSource;
        TVSchedule (required) : TSource;
        GeoMetaSearchOrganization (required) : TSource;
        GeoMetaSearchResolveText (required) : TSource;
        GeoMetaSearchResolveTextNextPage (required) : TSource;
        GeoMetaSearchReverseResolve (required) : TSource;
        GeoMetaSearchRoutePointContext (required) : TSource;
        GeoCoderLL2Geo (required) : TSource;
        GeoCoderText2RoadName (required) : TSource;
        Search (required) : TWebSearchSource;
        SerpSummarization (required) : TSource;
        SerpSummarizationAsync (required) : TSerpSummarizationSource;
        ReqWizard (required) : TSource;
        UnitsConverter (required) : TSource;
        CarRoutes (required) : TSource;
        CarsharingGreetingPhrase (required) : TSource;
        RouterVia (required) : TSource;
        MassTransitRoutes (required) : TSource;
        Passport (required) : TPassportSource;
        PedestrianRoutes (required) : TSource;
        PersonalData (required) : TSource;
        PersonalDataTest (required) : TSource;
        MapsInfoExportBackground (required) : TSource;
        NerApi (required) : TSource;
        News (required) : TSource;
        NewsApi (required) : TSource;
        NewsApiScheduler (required) : TSource;
        Music (required) : TSource;
        MusicQuasar (required) : TSource;
        MusicSuggests (required) : TSource;
        MusicCatalog (required) : TSource;
        MusicCatalogBulk (required) : TSource;
        MusicAvatarsColor (required) : TSource;
        RadioStream (required) : TSource;
        RadioStreamAvailableStations (required) : TSource;
        RadioStreamRecommendations (required) : TSource;
        TrafficForecastBackground (required) : TSource;
        Tvm2 (required) : TTvm2Source;
        UGCDb (required): TSource;
        VideoAmediateka (required): TSource;
        VideoHostingSeriesEpisodes (required): TSource;
        VideoHostingTvChannels (required): TSource;
        VideoHostingTvEpisodes (required): TSource;
        VideoHostingTvEpisodesAll (required): TSource;
        VideoHostingPersonalTvChannel (required) : TSource;
        VideoHostingPlayer (required) : TSource;
        VideoIvi (required): TSource;
        VideoKinopoisk (required): TKinopoiskSource;
        VideoKinopoiskUAPI (required) : TSource;
        VideoLsOtt (required) : TSource;
        VideoOkkoUAPI (required) : TSource;
        VideoYandexRecommendation (required): TSource;
        VideoYandexSearch (required): TSource;
        VideoYandexSearchOld (required): TSource;
        VideoYouTube (required): TSource;
        QuasarBillingPromoAvailability (required): TSource;
        QuasarBillingContentBuy (required): TSource;
        QuasarBillingContentPlay (required): TSource;
        QuasarBillingSkills (required): TSource;
        CloudApiDisk (required): TSource;
        CloudApiDiskUpload (required): TSource;
        ComputerVision (required): TSource;
        ComputerVisionClothes (required): TSource;
        ComputerVisionCbirFeatures (required): TSource;
        Market (required) : TSource;
        MarketHeavy (required) : TSource;
        MarketBlue (required) : TSource;
        MarketBlueHeavy (required) : TSource;
        MarketCheckouter (required) : TSource;
        MarketCheckouterHeavy (required) : TSource;
        MarketCheckouterIntervals (required) : TSource;
        MarketCheckouterOrders (required) : TSource;
        MarketFormalizer (required) : TSource;
        MarketMds (required) : TSource;
        MarketStockStorage (required) : TSource;
        MarketPersBasket (required) : TSource;
        MarketBeruMyBonusesList (required) : TSource;
        CalendarApi (required): TSource;
        RemindersApi (required): TSource;
        SocialApi : TSource;
        AviaBackend : TSource;
        AviaSuggests : TSource;
        AviaPriceIndex : TSource;
        AviaPriceIndexMinPrice : TSource;
        AviaTDApiInit : TSource;
        ExternalSkillsApi : TExternalSkill;
        ExternalSkillsDb (required) : TSource;
        ExternalSkillsKvSaaS : TSource;
        ExternalSkillsSaaS : TSource;
        ExternalSkillsRecommender : TExternalSkillsRecommender;
        GeneralConversationTurkish : TGeneralConversaion;
        ZoraProxy (required) : TZoraProxy;
        Translate : TSource;
        TranslateDict : TSource;
        TranslateMtAlice : TSource;
        TranslateTranslit : TSource;
        TranslateIsTranslit : TSource;
        NormalizedQuery : TSource;
        TankerApi : TSource;
        TaxiApiProxy : TSource;
        TaxiSupportApi : TSource;
        YandexFunctions (required) : TSource;
        YaRadioAccount (required) : TSource;
        YaRadioBackground (required) : TSource;
        YaRadioDashboard (required) : TSource;
        ZeroTesting (required) : TSource;
    };

    struct TCrmbot {
        Tvm2 (required) : TTvm2Source;
    };

    struct TRedirectApi {
        ClientId : string (required);
        Key      : string (required);
        Url      : string (required);
    };

    struct TMarketSettings {
        UseTestingUrls : bool (default = false);
        MaxCheckoutWaitDuration: duration (default = "15s");
        SearchThreads : ui16 (!= 0);
    };

    HttpPort (required) : ui16;
    HttpThreads : ui16 (default = 10);
    HttpConnections : ui32 (default = 500);
    SetupThreads : ui16 (required);
    SearchThreads : ui16 (!= 0);
    CacheUpdatePeriod : duration (default = "5m");

    EventLog : string;
    RTLog : TRTLogConfig;

    UpdateVideoContentDb : bool (default = false);
    VideoContentDbUpdatePeriod : duration (default = "1h");

    AvatarsMapFile : string;
    EnableBlobCards : bool (default = false);
    RedirectApi (required) : TRedirectApi;
    DialogsStoreUrl (required) : string;

    HamsterQuota : string;

    StaticMapApi (required) : struct {
        Url (required) : string;
        Key (required) : string;
    };

    Hosts (required) : struct {
        DialogsAuthorizationSkills (required) : string;
        QuasarBillingSkills (required) : string;
    };

    Vins (required) : TVins;
    Crmbot : TCrmbot;

    FetcherProxy : struct {
        HostPort (required) : string;
        Headers : [ THeader ];
    };
    OverrideAllTimeouts : duration (default = "0s");

    SkillStyles : { string -> any };

    PersonalizationNameDelay : duration (default = "3h");
    PersonalizationBiometryScoreThreshold : double (default = 0.9);
    PersonalizationAdditionalDataSyncTimeout : duration (default = "50ms");
    PersonalizationDropProbabilty : double (default = 0.66);

    PersonalizationMusicNamePronounceDelayPeriod : duration (default = "3h");
    PersonalizationMusicNamePronounceDelayCount : i64 (default = 3);
    PersonalizationMusicNamePronouncePeriod : i64 (default = 5);

    PushHandler (required) : struct {
        SupProvider : struct {
            Source : TSource;
            Token : string;
        };

        XivaProvider : struct {
            Source : TSource;
            Token : string;
        };
    };

    Market : TMarketSettings;

    YDb : TYdbConfig;
    TestUsersYDb : TYdbConfig;

    YdbConfigUpdatePeriod : duration (default = "5m");
    YdbKinopoiskSVODUpdatePeriod : duration (default = "5m");

    AliceFetcherNoRetry : bool (default = false);

    GeobasePath : string (default = "./geodata6.bin");
};
