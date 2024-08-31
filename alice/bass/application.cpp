#include "application.h"

#include "clients.h"
#include "http_request.h"
#include "server.h"
#include "ydb.h"
#include "ydb_config.h"

#include <alice/bass/forms/common/contacts/synonyms.h>
#include <alice/bass/forms/computer_vision/handler.h>
#include <alice/bass/forms/continuation_register.h>
#include <alice/bass/forms/geodb.h>
#include <alice/bass/forms/handlers.h>
#include <alice/bass/forms/music/cache.h>
#include <alice/bass/forms/navigation/bno_apps.h>
#include <alice/bass/forms/navigation/navigation.h>
#include <alice/bass/forms/news/news.h>
#include <alice/bass/forms/news/newsdata.h>
#include <alice/bass/forms/phone_call/phone_call.h>
#include <alice/bass/forms/request.h>
#include <alice/bass/forms/search/search.h>
#include <alice/bass/forms/translate/translate.h>
#include <alice/bass/forms/tv/channels_info.h>
#include <alice/bass/forms/urls_builder.h>
#include <alice/bass/forms/video/ivi_provider.h>
#include <alice/bass/forms/video/kinopoisk_content_snapshot.h>
#include <alice/bass/forms/video/kinopoisk_recommendations.h>
#include <alice/bass/forms/vins.h>

#include <alice/bass/libs/avatars/avatars.h>
#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/fetcher/fwd.h>
#include <alice/bass/libs/forms_db/delegate/delagate.h>
#include <alice/bass/libs/forms_db/forms_db.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/radio/fmdb.h>
#include <alice/bass/libs/rtlog/rtlog.h>
#include <alice/bass/libs/scheduler/bass_cache.h>
#include <alice/bass/libs/smallgeo/region.h>
#include <alice/bass/libs/source_request/source_request.h>
#include <alice/bass/libs/tvm2/tvm2_ticket_cache.h>
#include <alice/bass/libs/video_common/formulas.h>
#include <alice/bass/libs/video_common/ivi_genres.h>
#include <alice/bass/libs/ydb_config/config.h>

#include <alice/library/video_common/restreamed_data/restreamed_data.h>

#include <library/cpp/geobase/lookup.hpp>
#include <library/cpp/eventlog/eventlog.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/monlib/service/monservice.h>
#include <library/cpp/monlib/service/pages/registry_mon_page.h>
#include <library/cpp/monlib/service/pages/version_mon_page.h>
#include <library/cpp/sighandler/async_signals_handler.h>

#include <util/generic/noncopyable.h>
#include <util/stream/file.h>
#include <util/system/env.h>
#include <util/thread/pool.h>

#include <functional>

namespace {

using namespace NBASS::NSmallGeo;

class TIviGenresDelegate : public NVideoCommon::TIviGenres::TDelegate {
public:
    TIviGenresDelegate(const TSourcesRegistry& sources, const TConfig& config)
        : Factory(TSourcesRequestFactory{sources, config}) {
    }

    // NVideoCommon::TIviGenres::TDelegate overrides:
    THolder<NHttpFetcher::TRequest> MakeRequest(TStringBuf path) override {
        return Factory.Request(path);
    }

private:
    NBASS::NVideo::TIviSourceRequestFactory Factory;
};

NBASS::IGlobalContext::TSecrets ConstructSecrets() {
    static const TString GOOGLEAPIS_KEY_ENV("GOOGLEAPIS_KEY");
    static const TString KINOPOISK_TOKEN("KINOPOISK_TOKEN");
    static const TString TV_CLIENT_ID("TV_CLIENT_ID");
    static const TString TV_UID("TV_UID");
    static const TString YNAVIGATOR_KEY_ENV("YNAVIGATOR_KEY");
    static const TString TVM2_SECRET_ENV("TVM2_SECRET");
    static const TString STATICMAP_KEY_ENV("STATICMAP_ROUTER_KEY");

    return {
        .GoogleAPIsKey    = GetEnv(GOOGLEAPIS_KEY_ENV),
        .KinopoiskToken   = GetEnv(KINOPOISK_TOKEN),
        .TVClientId       = GetEnv(TV_CLIENT_ID),
        .TVUid            = GetEnv(TV_UID),
        .NavigatorKey     = GetEnv(YNAVIGATOR_KEY_ENV),
        .TVM2Secret       = GetEnv(TVM2_SECRET_ENV),
        .StaticMapRouterKey = GetEnv(STATICMAP_KEY_ENV),
    };
}

NGeobase::TLookup ConstructGeobaseLookup(const TString& geobasePath, bool lockGeobase) {
    Y_ENSURE(TFsPath(geobasePath).Exists(), "File " << geobasePath << " not found");
    // Default value for LockMemory is false
    return NGeobase::TLookup(geobasePath, NGeobase::TInitTraits{}.LockMemory(lockGeobase));
}

// FIXME use shortcut for TMonService
std::unique_ptr<NBASS::NMetrics::ICountersPlace::TMonService> ConstructAndRunMonService(NBASS::IGlobalContext& gctx) {
    auto monService =
        std::make_unique<NBASS::NMetrics::ICountersPlace::TMonService>(gctx.Config().GetMonitoringPort());
    gctx.Counters().Register(*monService);
    return monService;
}

std::unique_ptr<NBASS::ITVM2TicketCache> ConstructTVM2TicketCache(const TSourcesRegistry& sources,
                                                                  const TConfig& config,
                                                                  const NBASS::IGlobalContext::TSecrets& secrets,
                                                                  bool isMocked) {
    try {
        if (!isMocked) {
            LOG(INFO) << "Created real TVM 2.0 ticket cache" << Endl;
            return std::make_unique<NBASS::TTVM2TicketCache>(sources, config, secrets);
        }

        LOG(INFO) << "Created FAKE TVM 2.0 ticket cache" << Endl;
        return std::make_unique<NBASS::TFakeTvm2TicketCache>();
    } catch (yexception& e) {
        LOG(ERR) << "Construct tvm2 ticket cache problem: " << e.what() << Endl;
        return {};
    }
}

class TFormsDbDelegate : public IFormsDbDelegate {
public:
    TFormsDbDelegate(const TSourcesRegistry& sourcesRegistry, const TConfig& config, IScheduler& scheduler)
        : Config_(config)
        , Scheduler_(scheduler)
        , ExternalSkillsDbSF(sourcesRegistry.ExternalSkillsDb, config, /* path= */ {}, SourcesRegistryDelegate)
    {
    }

    const TConfig& Config() const override {
        return Config_;
    }

    IScheduler& Scheduler() override {
        return Scheduler_;
    }

    NHttpFetcher::TRequestPtr ExternalSkillDbRequest() const override {
        return ExternalSkillsDbSF.Request();
    }

private:
    const TConfig& Config_;
    IScheduler& Scheduler_;

    TDummySourcesRegistryDelegate SourcesRegistryDelegate;
    TSourceRequestFactory ExternalSkillsDbSF;
};

class TSourcesRegistryDelegate : public ISourcesRegistryDelegate {
public:
    TSourcesRegistryDelegate()
        : TVM2TicketCache(nullptr)
    {
    }

    void SetTVM2TicketCache(NBASS::ITVM2TicketCache* tvm2TicketCache) {
        TVM2TicketCache = tvm2TicketCache;
    }

    TMaybe<TString> GetTVM2ServiceTicket(TStringBuf serviceId) const override {
        if (TVM2TicketCache) {
            return TVM2TicketCache->GetTicket(serviceId);
        }
        return {};
    }

private:
    NBASS::ITVM2TicketCache* TVM2TicketCache;
};

class TGlobalContext : public NBASS::IGlobalContext, NNonCopyable::TNonCopyable {
private:
    class TCounters : public NBASS::NMetrics::ICountersPlace, NNonCopyable::TNonCopyable {
    public:
        TCounters()
            : Sensors_(MakeAtomicShared<NMonitoring::TMetricRegistry>())
            , SkillSensors_(MakeAtomicShared<NMonitoring::TMetricRegistry>())
            , BassCounters_(MakeIntrusive<NMonitoring::TBassCounters>())
        {
            NMonitoring::SetSensorsSingleton(Sensors_);
            NMonitoring::TBassCounters::SetCountersSingleton(BassCounters_);
        }

        // NBASS::NMetrics::ICountersPlace overrides:
        NMonitoring::TMetricRegistry& Sensors() override {
            return *Sensors_;
        }

        NMonitoring::TMetricRegistry& SkillSensors() override {
            return *SkillSensors_;
        }

        NMonitoring::TBassCounters& BassCounters() override {
            return *BassCounters_;
        }

        const NBASS::NMetrics::TSignals& Signals() const override {
            return Signals_;
        }

        void Register(TMonService& service) override {
            using TSignalType = NBASS::NMetrics::TSignalDescr::EType;
            using TSignalFlag = NBASS::NMetrics::TSignalDescr::EFlag;
            Signals_.Register(*this, { "bass_request_report", TSignalType::Histogram, (TSignalFlag::Solomon | TSignalFlag::UniStat) });
            Signals_.Register(*this, { "bass_request_setup", TSignalType::Histogram, (TSignalFlag::Solomon | TSignalFlag::UniStat) });

            service.Register(new NMonitoring::TVersionMonPage);

            service.Register(new NMonitoring::TBassGolovanCountersPage("stat", Sensors_));
            service.Register(new NMonitoring::TMetricRegistryPage("counters", "Counters", Sensors_));
            service.Register(new NMonitoring::TMetricRegistryPage("skillCounters", "SkillCounters", SkillSensors_));
        }

    private:
        using TDynamicCountersPtr = TIntrusivePtr<NMonitoring::TDynamicCounters>;
        NMonitoring::TMetricRegistryPtr Sensors_;
        NMonitoring::TMetricRegistryPtr SkillSensors_;
        TIntrusivePtr<NMonitoring::TBassCounters> BassCounters_;
        NBASS::NMetrics::TSignals Signals_;
    };

public:
    TGlobalContext(const TString& configFile, ui16 port, NBASS::EBASSClient client, const TConfig::TKeyValues& cmdVariables, bool lockGeobase, const TString& currentDc, bool isMocked = false)
        : Config_(TFileInput(configFile).ReadAll(), port, cmdVariables, Nothing() /* jsonPatcher */, client, currentDc)
        , EventLog_(Config().HasEventLog() ? TEventLog(TString{*Config().EventLog()}, NEvClass::Factory()->CurrentFormat()) : TEventLog(NEvClass::Factory()->CurrentFormat()))
        , RTLogClient_(NBASS::ConstructRTLogClient(Config().RTLog()))
        , YdbConfig_(Config_)
        , YdbDriver_(NBASS::ConstructYdbDriver(Config_))
        , YdbClient_(YdbDriver_)
        , Secrets_(ConstructSecrets())
        , AvatarsMap_(Config().HasAvatarsMapFile() ? TAvatarsMap(Config().AvatarsMapFile()) : TAvatarsMap())
        , Counters_(std::make_unique<TCounters>())
        , Scheduler_(std::make_unique<NBASS::TCacheManager>())
        , SourcesRegistryDelegate_()
        , Sources_(*Counters_, SourcesRegistryDelegate_)
        , TVM2TicketCache_(ConstructTVM2TicketCache(Sources_, Config_, Secrets_, isMocked))
        , IviGenresDelegate_(Sources_, Config_)
        , IviGenres_(IviGenresDelegate_)
        , FormsDbDelegate_(Sources_, Config_, *Scheduler_)
        , FormsDb_(FormsDbDelegate_)
        , GeobaseLookup_{ConstructGeobaseLookup(TString{*Config().GeobasePath()}, lockGeobase)}
    {
        switch (Config_.BASSClient()) {
            case NBASS::EBASSClient::Alice:
                FormsDb_.InitExternalSkillsDb();
                NewsData_.ConstructInPlace(NBASS::NNews::TNewsData::Create(*this));
                NBASS::NNews::InitLemmer();
                RadioStationCache_.ConstructInPlace(NBASS::NMusic::TStationsData(*this));
                break;
            case NBASS::EBASSClient::DevNoMusic:
                FormsDb_.InitExternalSkillsDb();
                NewsData_.ConstructInPlace(NBASS::NNews::TNewsData::Create(*this));
                NBASS::NNews::InitLemmer();
                break;
            case NBASS::EBASSClient::Crmbot:
                break;
        }
        SourcesRegistryDelegate_.SetTVM2TicketCache(TVM2TicketCache_.get());
        Handlers_ = std::make_unique<NBASS::THandlersMap>(*this);

        MessageBusThreadPool_.Start(Config().HttpThreads() /* thread count */, 1 /* queue size */);
        MarketThreadPool_.Start(Config().HasMarket() ? Config().Market().SearchThreads() : 1 /* thread count */, 0 /* queue size */);
        AviaThreadPool_.Start(1, 100);
    }

    ~TGlobalContext() {
        YdbDriver_.Stop(true /* wait */);
    }

    // NBASS::IGlobalContext overrides:
    IEventLog& EventLog() override {
        return EventLog_;
    }

    NRTLog::TClient& RTLogClient() override {
        return RTLogClient_;
    }

    const TConfig& Config() const override {
        return Config_;
    }

    const TSecrets& Secrets() const override {
        return Secrets_;
    }

    const TAvatarsMap& AvatarsMap() const override {
        return AvatarsMap_;
    }

    const ISourcesRegistryDelegate& SourcesRegistryDelegate() const override {
        return SourcesRegistryDelegate_;
    }

    const TSourcesRegistry& Sources() const override {
        return Sources_;
    }

    NBASS::NMetrics::ICountersPlace& Counters() override {
        return *Counters_;
    }

    IScheduler& Scheduler() override {
        return *Scheduler_;
    }

    const NBASS::THandlersMap::THandlerFactory* ActionHandler(TStringBuf action) const override {
        return Handlers_->ActionHandler(action);
    }

    const NBASS::THandlersMap::THandlerFactory* FormHandler(TStringBuf form) const override {
        return Handlers_->FormHandler(form);
    }

    const NBASS::THandlersMap::TContinuableHandlerFactory* ContinuableHandler(TStringBuf form) const override {
        return Handlers_->ContinuableHandler(form);
    }

    TDuration ActionSLA(TStringBuf action) const override {
        return Handlers_->ActionSLA(action);
    }

    TDuration FormSLA(TStringBuf form) const override {
        return Handlers_->FormSLA(form);
    }

    NVideoCommon::TIviGenres& IviGenres() override {
        return IviGenres_;
    }

    const NBASS::TFormsDb& FormsDb() const override {
        return FormsDb_;
    }

    NBASS::TYdbConfig& YdbConfig() override {
        return YdbConfig_;
    }

    NYdb::NTable::TTableClient& YdbClient() override {
        return YdbClient_;
    }

    NBASS::ITVM2TicketCache* TVM2TicketCache() override {
        return TVM2TicketCache_.get();
    }

    bool IsTVM2Expired() override {
        auto* cache = TVM2TicketCache();
        return !cache || cache->SinceLastUpdate() > Config().Tvm2().UpdatePeriod() + TDuration::Minutes(1);
    }

    IThreadPool& MessageBusThreadPool() override {
        return MessageBusThreadPool_;
    }

    IThreadPool& MarketThreadPool() override {
        return MarketThreadPool_;
    }

    NBASS::TContinuationParserRegistry& ContinuationRegistry() override {
        return ContinuationRegistry_;
    }

    const NGeobase::TLookup& GeobaseLookup() const override {
        return GeobaseLookup_;
    }

    const NBASS::NMusic::TStationsData& RadioStations() const override {
        return *RadioStationCache_;
    }

    const NBASS::NNews::TNewsData& NewsData() const override {
        return *NewsData_;
    }

    IThreadPool& AviaThreadPool() override {
        return AviaThreadPool_;
    }

protected:
    void DoInitHandlers() override {
        auto handlers = std::make_unique<NBASS::THandlersMap>(static_cast<NBASS::IGlobalContext&>(*this));
        NBASS::RegisterHandlers(*handlers, *this);
        Handlers_ = std::move(handlers);

        NBASS::RegisterContinuations(ContinuationRegistry_);
    }

private:
    const TConfig Config_;
    TEventLog EventLog_;
    NRTLog::TClient RTLogClient_;

    NBASS::TYdbConfig YdbConfig_;
    NYdb::TDriver YdbDriver_;
    NYdb::NTable::TTableClient YdbClient_;

    const NBASS::IGlobalContext::TSecrets Secrets_;
    const TAvatarsMap AvatarsMap_;

    std::unique_ptr<TCounters> Counters_;
    std::unique_ptr<IScheduler> Scheduler_;

    TSourcesRegistryDelegate SourcesRegistryDelegate_;
    TSourcesRegistry Sources_;
    std::unique_ptr<NBASS::ITVM2TicketCache> TVM2TicketCache_;

    TIviGenresDelegate IviGenresDelegate_;
    NVideoCommon::TIviGenres IviGenres_;

    TFormsDbDelegate FormsDbDelegate_;
    NBASS::TFormsDb FormsDb_;

    std::unique_ptr<NBASS::THandlersMap> Handlers_;
    TThreadPool MessageBusThreadPool_;
    TThreadPool MarketThreadPool_;
    TThreadPool AviaThreadPool_;

    NBASS::TContinuationParserRegistry ContinuationRegistry_;

    NGeobase::TLookup GeobaseLookup_;

    TMaybe<NBASS::NMusic::TStationsData> RadioStationCache_;
    TMaybe<NBASS::NNews::TNewsData> NewsData_;
};

} // namespace

TApplication* TApplication::Instance = nullptr;

TApplication::TApplication(int argc, const char** argv) {
    Y_ENSURE(!Instance);
    Instance = this;

    NLastGetopt::TOpts opts;
    opts.SetFreeArgsMax(1);
    opts.SetFreeArgTitle(0, "<config.json>", "configuration file in json, default = \"configs/localhost_config.json\"");

    ui16 httpPort = 0;
    opts.AddLongOption('p', "port").Help("Http port").Optional().StoreResult(&httpPort);

    TString logDir;
    opts.AddLongOption("logdir").Help("Logdir or console").DefaultValue("console").StoreResult(&logDir);

    ELogPriority logLevel;
    opts.AddLongOption("loglevel").Help("Log level").DefaultValue("DEBUG").StoreResultT<ELogPriority>(&logLevel);

    NBASS::EBASSClient client;
    opts.AddLongOption("client").Help("Vins client app for this BASS instance").DefaultValue("Alice").StoreResult(&client);

    TConfig::TKeyValues vars;
    opts.AddCharOption('V', "KEY=VALUE, set config Lua variable key to value").RequiredArgument().AppendTo(&vars);

    bool isMocked;
    opts.AddCharOption('F', "Mock TVM service").Optional().NoArgument().DefaultValue(false).StoreTrue(&isMocked);

    bool lockGeobase;
    opts.AddLongOption("lock-geobase", "Lock geobase in memory").Optional().NoArgument().DefaultValue(false).StoreTrue(&lockGeobase);

    TString currentDc;
    opts.AddLongOption("current-dc", "set current dc").Optional().StoreResult(&currentDc);

    opts.AddLongOption("skip-waiting-first-run-dangerous", "do not wait for first run (can be dangerous, only for developers)").Optional().NoArgument().StoreValue(&SkipWaitingFirstRun, true);
    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    const bool enableTSUMTrace = (client == NBASS::EBASSClient::Crmbot);
    TLogging::Configure(logDir, httpPort, logLevel, enableTSUMTrace);

    TVector<TString> freeArgs = res.GetFreeArgs();
    TString configFile;
    if (freeArgs.empty()) {
        configFile = "configs/localhost_config.json";
    } else {
        configFile = freeArgs[0];
    }

    GlobalContext = NBASS::IGlobalContext::MakePtr<TGlobalContext>(configFile, httpPort, client, vars, lockGeobase, currentDc, isMocked);
    // TODO move it to the Run() ???
    MonService = ConstructAndRunMonService(*GlobalContext);

    InitPreTVM2();

    InitTVM2();

    InitPostTVM2();

    GlobalContext->InitHandlers();

    // Creates and starts http server.
    HttpServer = std::make_unique<TBassServer>(GlobalContext);

    // Remove these signal handlers in d-tor.
    auto shutdown = [](int) {
        TApplication::GetInstance()->Shutdown();
    };
    SetAsyncSignalFunction(SIGTERM, shutdown);
    SetAsyncSignalFunction(SIGINT, shutdown);

    // TODO(yakovdom): remove SIGHUB handler after changing configs in nanny
    auto rotate = [](int) {
        TApplication::GetInstance()->ReopenLogs();
    };
    SetAsyncSignalFunction(SIGHUP, rotate);
}

TApplication::~TApplication() {
    GlobalContext->Scheduler().Shutdown();

    // Remove signal handlers to don't use TApplication after it was deleted.
    SetAsyncSignalFunction(SIGTERM, nullptr);
    SetAsyncSignalFunction(SIGINT, nullptr);

    if (Instance == this)
        Instance =  nullptr;

    // Wait for HTTP server for monitoring to shutdown properly to avoid race condition.
    MonService->Wait();
}

void TApplication::ReopenLogs() {
    LOG(INFO) << "logs rotated" << Endl;
    TLogging::Rotate();
    GlobalContext->EventLog().ReopenLog();
}

void TApplication::Run() {
    if (IsRunning) {
        return;
    }
    IsRunning = true;

    while (!MonService->Start()) {
        if (AtomicGet(IsShutdownInitiated)) {
            return;
        }
        LOG(INFO) << "Starting monitoring HTTP server..." << Endl;
        Sleep(TDuration::Seconds(1));
    }
    LOG(INFO) << "Monitoring started on port " << GlobalContext->Config().GetMonitoringPort() << Endl;

    if (!SkipWaitingFirstRun) {
        GlobalContext->Scheduler().WaitFirstRun();
    }

    // XXX move it to global context???
    auto& bassCounters = GlobalContext->Counters().BassCounters();
    bassCounters.RegisterUnistatHistogram("http_request_vins_timings");
    bassCounters.RegisterUnistatHistogram("http_request_wait_in_queue");
    bassCounters.InitUnistat();

    while (!HttpServer->Start()) {
        if (AtomicGet(IsShutdownInitiated)) {
            return;
        }
        LOG(INFO) << "starting HTTP server..." << Endl;
        Sleep(TDuration::Seconds(1));
    }
    LOG(INFO) << "HTTP server started on port " << GlobalContext->Config().GetHttpPort() << Endl;

    HttpServer->Wait();
    LOG(INFO) << "HTTP server stopped" << Endl;
}

void TApplication::Shutdown() {
    AtomicSet(IsShutdownInitiated, true);

    GlobalContext->Scheduler().Shutdown();

    LOG(INFO) << "HTTP server shutdown initiated" << Endl;
    HttpServer->Shutdown();

    MonService->Shutdown();
}

void TApplication::ReloadYaStrokaFixList() {
    NBASS::TNavigationFixList::Instance()->ReloadYaStrokaFixList();
}

void TApplication::ReloadBnoApps() {
    NBASS::TBnoApps::Instance()->Reload();
}

const TSourcesRegistry& TApplication::GetSources() const {
    return GlobalContext->Sources();
}

const TConfig& TApplication::GetConfig() const {
    return GlobalContext->Config();
}

void TApplication::InitPreTVM2() {
    switch (GlobalContext->Config().BASSClient()) {
        case NBASS::EBASSClient::Alice:
        case NBASS::EBASSClient::DevNoMusic:
            InitPreTVM2Alice();
            break;
        case NBASS::EBASSClient::Crmbot:
            break;
    }
}

void TApplication::InitPreTVM2Alice() {
    NBASS::TNavigationFormHandler::Init();
    NBASS::TSearchFormHandler::Init();
    NBASS::TComputerVisionMainHandler::Init();
    NBASS::NVideo::InitKpContentSnapshot();
    NBASS::NSmallGeo::TRegions::Instance();

    NVideoCommon::TFormulas::Instance()->Init();

    // TODO move to a separate init function (which must be created) for all such methods
    NBASS::TPhoneCallHandler::GetEmergencyPhoneBook(); // it loads emergency phone numbers for different countries
    NBASS::InitAppleSchemes(); // it loads iOS application schemes from the file for usage in urls_builder for all
                               // forms
    // load restreamed TV-channels data before creation tv-channels cache
    NAlice::NVideoCommon::TRestreamedChannelsData::Instance().Init();
    NBASS::TTvChannelsInfoCache::Init(*GlobalContext);

    NBASS::TClusters::Instance().Init();

    // It's important to pull initial values from config table.
    GlobalContext->YdbConfig().Update();
    GlobalContext->Scheduler().Schedule([this]() {
        GlobalContext->YdbConfig().Update();
        return GlobalContext->Config().YdbConfigUpdatePeriod();
    });
}

void TApplication::InitTVM2() {
    if (auto* ticketCache = GlobalContext->TVM2TicketCache()) {
        GlobalContext->Scheduler().Schedule([this, ticketCache]() {
          const auto success = ticketCache->Update();
          if (!success) {
              Y_STATS_INC_COUNTER("Tvm2TicketCache_Update_error");
              return TDuration::Seconds(5);
          }
          Y_STATS_INC_COUNTER("Tvm2TicketCache_Update_success");
          return GlobalContext->Config().Tvm2().UpdatePeriod().Get();
        });
    }
}

void TApplication::InitPostTVM2() {
    switch (GlobalContext->Config().BASSClient()) {
        case NBASS::EBASSClient::Alice:
        case NBASS::EBASSClient::DevNoMusic:
            InitPostTVM2Alice();
            break;
        case NBASS::EBASSClient::Crmbot:
            break;
    }
}

void TApplication::InitPostTVM2Alice() {
    NBASS::NVideo::TKinopoiskRecommendations::Instance().SetUpdatePeriod(
        GlobalContext->Config().YdbKinopoiskSVODUpdatePeriod());
    GlobalContext->Scheduler().Schedule(
        [this]() { return NBASS::NVideo::TKinopoiskRecommendations::Instance().Update(GlobalContext); });
}
