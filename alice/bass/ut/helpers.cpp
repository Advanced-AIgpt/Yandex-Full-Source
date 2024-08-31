#include "helpers.h"

#include <alice/bass/libs/tvm2/tvm2_ticket_cache.h>
#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/geodb.h>
#include <alice/bass/forms/music/cache.h>
#include <alice/bass/forms/news/newsdata.h>
#include <alice/bass/forms/unit_test_form.h>
#include <alice/bass/forms/vins.h>
#include <alice/bass/libs/avatars/avatars.h>
#include <alice/bass/libs/forms_db/delegate/delagate.h>
#include <alice/bass/libs/forms_db/forms_db.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/rtlog/rtlog.h>
#include <alice/bass/libs/scheduler/scheduler.h>
#include <alice/bass/libs/source_request/source_request.h>
#include <alice/bass/libs/video_common/video_ut_helpers.h>
#include <alice/bass/ydb.h>
#include <alice/bass/ydb_config.h>

#include <alice/library/json/json.h>

#include <library/cpp/geobase/lookup.hpp>
#include <library/cpp/resource/resource.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/stream/file.h>

using namespace NBASS;

namespace NTestingHelpers {
namespace {

constexpr TStringBuf DEFAULT_REQUEST_JSON_META = TStringBuf(R"-(
{
   "meta" : {
      "client_id" : "ru.yandex.searchplugin/7.90 (Xiaomi Redmi Note 4; android 7.1.2)",
      "epoch" : 1538335437,
      "user_agent" : "Mozilla/5.0 (Linux; Android 7.1.2; Redmi Note 4 Build/NJH47F; wv) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/66.0.3359.139 Mobile Safari/537.36 YandexSearch/7.90",
      "tz" : "Europe/Moscow",
      "uuid" : "deadbeefdeadbeefdeadbeefdeadbeef"
   }
})-");

template <typename T>
struct TLazy {
    T& Get() {
        if (!Value) {
            Value.ConstructInPlace();
        }
        return *Value;
    }

    TMaybe<T> Value;
};

TContext::TPtr MakeContextImpl(const NSc::TValue& request, TContext::TInitializer init,
                               bool shouldValidateRequest = true) {
    if (const auto& event = request["meta"]["event"]; !event.IsNull()) {
        init.SpeechKitEvent = NAlice::JsonToProto<NAlice::TEvent>(event.ToJsonValue());
    }
    TContext::TPtr ctx;
    const auto error = shouldValidateRequest ? TContext::FromJson(request, init, &ctx)
                                             : TContext::FromJsonUnsafe(request, init, &ctx);
    UNIT_ASSERT_C(!error.Defined(), error->Msg);
    UNIT_ASSERT(ctx);
    return ctx;
}

} // namespace

void CheckResponse(const TContext& ctx, const NSc::TValue& expected) {
    NSc::TValue actual;
    ctx.ToJson(&actual);
    UNIT_ASSERT(EqualJson(expected, actual));
}

TContext::TPtr MakeContext(const NSc::TValue& request, bool shouldValidateRequest, const TMaybe<TString>& userTicket) {
    TGlobalContextPtr globalCtx = IGlobalContext::MakePtr<TTestGlobalContext>();
    return MakeContextImpl(request,
                           {globalCtx,
                            "somereqid",
                            {} /* authHeader */,
                            {} /* appInfoHeader */,
                            {} /* fakeTimeHeader */,
                            userTicket,
                            {} /* SpeechKitEvent */},
                           shouldValidateRequest);
}

TContext::TPtr MakeContext(TStringBuf request, bool shouldValidateRequest, const TMaybe<TString>& userTicket) {
    return MakeContext(NSc::TValue::FromJsonThrow(request), shouldValidateRequest, userTicket);
}

TContext::TPtr MakeAuthorizedContext(const NSc::TValue& request) {
    TGlobalContextPtr globalCtx = IGlobalContext::MakePtr<TTestGlobalContext>();
    return MakeContextImpl(request, {globalCtx,
                                     "somereqid",
                                     "some_token",
                                     "" /* appInfoHeader */,
                                     {} /* fakeTimeHeader */,
                                     Nothing() /* userTicketHeader */,
                                     {} /* SpeechKitEvent */});
}

TContext::TPtr MakeAuthorizedContext(TStringBuf request) {
    return MakeAuthorizedContext(NSc::TValue::FromJsonThrow(request));
}

TContext::TPtr CreateVideoContextWithAgeRestriction(EContentRestrictionLevel restriction,
                                                    std::function<TContext::TPtr(const NSc::TValue&)> contextCreator) {
    const auto VIDEO_PLAY_DUMMY_REQUEST = NSc::TValue::FromJson(R"(
        {
            "form": {
                "name": "personal_assistant.scenarios.video_play",
                "slots": []
            },
            "meta": {
                "epoch": 1526559187,
                "tz": "Europe/Moscow",
                "device_state": {
                    "device_config": {
                        "content_settings": "children"
                    },
                    "is_tv_plugged_in": true
                }
            }
        }
        )");

    NSc::TValue context = VIDEO_PLAY_DUMMY_REQUEST;
    context["meta"]["device_state"]["device_config"]["content_settings"].SetString(ToString(restriction));
    return contextCreator(context);
}

const TConfig& ConstructConfig(TStringBuf fileName, TMaybe<TConfig::TOnJsonPatch> configJsonPatcher) {
    static THashMap<TString, TConfig> configs;

    const auto* config = configs.FindPtr(fileName);
    if (!config) {
        TConfig cfg{TFileInput{TString{fileName}}.ReadAll(), 0/* http port */, TConfig::TKeyValues(), configJsonPatcher};
        auto [ it, exists ] = configs.emplace(fileName, std::move(cfg));
        config = &it->second;
    }

    Y_ASSERT(config);

    return *config;
}

class TTestFromsDbDelegate : public IFormsDbDelegate {
public:
    TTestFromsDbDelegate(const TConfig& config, IScheduler& scheduler)
        : Config_(config)
        , Scheduler_(scheduler)
    {
    }

    const TConfig& Config() const override {
        return Config_;
    }

    IScheduler& Scheduler() override {
        return Scheduler_;
    }

    NHttpFetcher::TRequestPtr ExternalSkillDbRequest() const override {
        return nullptr;
    }

private:
    const TConfig& Config_;
    IScheduler& Scheduler_;
};

class TDummyScheduler : public IScheduler {
public:
    void Schedule(TJob, TDuration = TDuration::Zero()) override {
    }

    void WaitFirstRun() override {
    }

    void Shutdown() override {
    }
};

class TTestGlobalContext::TImpl {
public:
    explicit TImpl(TStringBuf config, TMaybe<TConfig::TOnJsonPatch> configJsonPatcher)
        : Config(ConstructConfig(config, configJsonPatcher))
        , IviGenres(IviGenresDelegate)
        , RTLogClient(NBASS::ConstructRTLogClient(Config.RTLog()))
        , Scheduler()
        , FormsDbDelegate(Config, Scheduler)
        , FormsDb(FormsDbDelegate)
    {
    }

    class TTestCounters : public NMetrics::ICountersPlace {
    public:
        TTestCounters()
            : Sensors_(MakeAtomicShared<NMonitoring::TMetricRegistry>())
            , SkillSensors_(MakeAtomicShared<NMonitoring::TMetricRegistry>())
        {
        }

        NMonitoring::TMetricRegistry& Sensors() override {
            return *Sensors_;
        }

        NMonitoring::TMetricRegistry& SkillSensors() override {
            return *SkillSensors_;
        }


        NMonitoring::TBassCounters& BassCounters() override {
            return BassCounters_;
        }

        const NMetrics::TSignals& Signals() const override {
            return Signals_;
        }

        void Register(TMonService&) override {
        }

    private:
        NMonitoring::TMetricRegistryPtr Sensors_;
        NMonitoring::TMetricRegistryPtr SkillSensors_;
        NMonitoring::TBassCounters BassCounters_;
        NMetrics::TSignals Signals_;
    };

    NYdb::TDriver& GetOrCreateYdbDriver() {
        if (!YdbDriver)
            YdbDriver.ConstructInPlace(NYdb::TDriverConfig());
        return *YdbDriver;
    }

    TLazy<TSecrets> Secrets;
    TLazy<TAvatarsMap> AvatarsMap;
    TDummySourcesRegistryDelegate SourcesRegistryDelegate;
    TMaybe<TSourcesRegistry> Sources;
    TLazy<TTestCounters> Counters;

    const TConfig& Config;

    NTestingHelpers::TIviGenresDelegate IviGenresDelegate;
    NVideoCommon::TIviGenres IviGenres;

    TMaybe<NMusic::TStationsData> RadioStations;
    TMaybe<NNews::TNewsData> NewsData;

    TMaybe<TYdbConfig> YdbConfig;
    TMaybe<NYdb::TDriver> YdbDriver;
    TMaybe<NYdb::NTable::TTableClient> YdbClient;
    NRTLog::TClient RTLogClient;

    TDummyScheduler Scheduler;

    TTestFromsDbDelegate FormsDbDelegate;
    NBASS::TFormsDb FormsDb;

    THolder<NBASS::ITVM2TicketCache> TVM2TicketCache;
    bool TVM2TicketCacheCreationFailed = false;

    TMaybe<TAdaptiveThreadPool> MessageBusThreadPool;
    TMaybe<TAdaptiveThreadPool> MarketThreadPool;
    TMaybe<TAdaptiveThreadPool> AviaThreadPool;

    TContinuationParserRegistry ContinuationRegistry;
};

// TTestGlobalContext ---------------------------------------------------------
TTestGlobalContext::TTestGlobalContext(TStringBuf pathToConfig)
    : Impl_{MakeHolder<TImpl>(pathToConfig, Nothing())}
{
}

TTestGlobalContext::TTestGlobalContext(TConfig::TOnJsonPatch configJsonPatcher, TStringBuf pathToConfig)
    : Impl_{MakeHolder<TImpl>(pathToConfig, configJsonPatcher)}
{
}

TTestGlobalContext::~TTestGlobalContext() {
    if (Impl_->YdbDriver)
        Impl_->YdbDriver->Stop(true /* wait */);
}

NMetrics::ICountersPlace& TTestGlobalContext::Counters() {
    return Impl_->Counters.Get();
}

const TAvatarsMap& TTestGlobalContext::AvatarsMap() const {
    return Impl_->AvatarsMap.Get();
}

const ISourcesRegistryDelegate& TTestGlobalContext::SourcesRegistryDelegate() const {
    return Impl_->SourcesRegistryDelegate;
}

const TSourcesRegistry& TTestGlobalContext::Sources() const {
    if (!Impl_->Sources) {
        Impl_->Sources.ConstructInPlace(Impl_->Counters.Get(), Impl_->SourcesRegistryDelegate);
    }
    return *Impl_->Sources;
}

NVideoCommon::TIviGenres& TTestGlobalContext::IviGenres() {
    return Impl_->IviGenres;
}

const NBASS::TFormsDb& TTestGlobalContext::FormsDb() const {
    return Impl_->FormsDb;
}

// The methods below will cause an exception if used. One who needs them has to implement fast data mocking.
const NBASS::NMusic::TStationsData& TTestGlobalContext::RadioStations() const {
    return *Impl_->RadioStations;
}

const NBASS::NNews::TNewsData& TTestGlobalContext::NewsData() const {
    return *Impl_->NewsData;
}

TYdbConfig& TTestGlobalContext::YdbConfig() {
    if (!Impl_->YdbConfig)
        Impl_->YdbConfig.ConstructInPlace(Config());
    return *Impl_->YdbConfig;
}

NYdb::NTable::TTableClient& TTestGlobalContext::YdbClient() {
    if (!Impl_->YdbClient)
        Impl_->YdbClient.ConstructInPlace(Impl_->GetOrCreateYdbDriver());
    return *Impl_->YdbClient;
}

const NBASS::IGlobalContext::TSecrets& TTestGlobalContext::Secrets() const {
    return Impl_->Secrets.Get();
}

IEventLog& TTestGlobalContext::EventLog() {
    class TTestEventLog : public TEventLog {
    public:
        TTestEventLog()
            : TEventLog(NEvClass::Factory()->CurrentFormat())
        {
        }

        using TEventLog::LogEvent;
    };
    return *Singleton<TTestEventLog>();
}

NRTLog::TClient& TTestGlobalContext::RTLogClient() {
    return Impl_->RTLogClient;
}

const TConfig& TTestGlobalContext::Config() const {
    return Impl_->Config;
}

IScheduler& TTestGlobalContext::Scheduler() {
    return Impl_->Scheduler;
}

NBASS::ITVM2TicketCache* TTestGlobalContext::TVM2TicketCache() {
    if (!Impl_->TVM2TicketCache && !Impl_->TVM2TicketCacheCreationFailed) {
        try {
            Impl_->TVM2TicketCache = MakeHolder<NBASS::TTVM2TicketCache>(Sources(), Config(), Secrets());
        } catch (const yexception& e) {
            LOG(ERR) << "Failed to create TVM 2.0 ticket cache: " << e << Endl;
            Impl_->TVM2TicketCacheCreationFailed = true;
        }
    }
    return Impl_->TVM2TicketCache.Get();
}

bool TTestGlobalContext::IsTVM2Expired() {
    auto* cache = TVM2TicketCache();
    return !cache || cache->SinceLastUpdate() > Config().Tvm2().UpdatePeriod() + TDuration::Minutes(1);
}

IThreadPool& TTestGlobalContext::MessageBusThreadPool() {
    if (!Impl_->MessageBusThreadPool.Defined()) {
        Impl_->MessageBusThreadPool.ConstructInPlace();
        Impl_->MessageBusThreadPool->Start(0 /* threads number */, 0 /* queue size */); // both arguments are ignored
    }
    return Impl_->MessageBusThreadPool.GetRef();
}

IThreadPool& TTestGlobalContext::MarketThreadPool() {
    if (!Impl_->MarketThreadPool.Defined()) {
        Impl_->MarketThreadPool.ConstructInPlace();
        Impl_->MarketThreadPool->Start(0 /* threads number */, 0 /* queue size */); // both arguments are ignored
    }
    return Impl_->MarketThreadPool.GetRef();
}

IThreadPool& TTestGlobalContext::AviaThreadPool() {
    if (!Impl_->AviaThreadPool.Defined()) {
        Impl_->AviaThreadPool.ConstructInPlace();
        Impl_->AviaThreadPool->Start(0 /* threads number */, 0 /* queue size */); // both arguments are ignored
    }
    return Impl_->AviaThreadPool.GetRef();
}

TContinuationParserRegistry& TTestGlobalContext::ContinuationRegistry() {
    return Impl_->ContinuationRegistry;
}

NBASS::TContext::TPtr TBassContextFixture::MakeContext(const NSc::TValue& request) {
    return MakeContextImpl(request, {GlobalCtx(),
                                     "somereqid",
                                     "" /* authHeader */,
                                     "" /* appInfoHeader */,
                                     {} /* fakeTimeHeader */,
                                     Nothing() /* userTicketHeader */,
                                     {} /* SpeechKitEvent */});
}

NBASS::TContext::TPtr TBassContextFixture::MakeContext(TStringBuf request) {
    return MakeContextImpl(NSc::TValue::FromJsonThrow(request), {GlobalCtx(),
                                                                 "somereqid",
                                                                 "" /* authHeader */,
                                                                 "" /* appInfoHeader */,
                                                                 {} /* fakeTimeHeader */,
                                                                 Nothing() /* userTicketHeader */,
                                                                 {} /* SpeechKitEvent */});
}

NBASS::TContext::TPtr TBassContextFixture::MakeAuthorizedContext(const NSc::TValue& request) {
    return MakeContextImpl(request, {GlobalCtx(),
                                     "somereqid",
                                     "some_token",
                                     "" /* appInfoHeader */,
                                     {} /* fakeTimeHeader */,
                                     Nothing() /* userTicketHeader */,
                                     {} /* SpeechKitEvent */});
}

NBASS::TContext::TPtr TBassContextFixture::MakeAuthorizedContext(TStringBuf request) {
    return MakeContextImpl(NSc::TValue::FromJsonThrow(request), {GlobalCtx(),
                                                                 "somereqid",
                                                                 "some_token",
                                                                 "" /* appInfoHeader */,
                                                                 {} /* fakeTimeHeader */,
                                                                 Nothing() /* userTicketHeader */,
                                                                 {} /* SpeechKitEvent */});
}

TRequestJson TBassContextFixture::CreateDefaultRequestJson() const {
    return TRequestJson{};
}

TConfig::TYdbScheme TBassContextFixture::LocalYdbConfig() {
    if (!LocalYdbConfig_) {
        NSc::TValue config;
        config["DataBase"].SetString(TFileInput("ydb_database.txt").ReadLine());
        config["Endpoint"].SetString(TFileInput("ydb_endpoint.txt").ReadLine());
        LocalYdbConfig_.ConstructInPlace(std::move(config));
    }
    return TConfig::TYdbScheme(LocalYdbConfig_.Get());
}

NYdb::TDriver& TBassContextFixture::LocalYdb() {
    if (!LocalDriver_) {
        const auto config{LocalYdbConfig()};
        LocalDriver_.ConstructInPlace(NYdb::TDriverConfig().SetEndpoint(TString{*config.Endpoint()}).SetDatabase(TString{*config.DataBase()}));
    }

    return *LocalDriver_;
}

NBASS::TGlobalContextPtr TBassContextFixture::GlobalCtx() {
    if (!GlobalCtx_) {
        GlobalCtx_ = MakeGlobalCtx();
        UNIT_ASSERT_C(GlobalCtx_, "Unable to construct global context");
    }
    return GlobalCtx_;
}

const NGeobase::TLookup& TTestGlobalContext::GeobaseLookup() const {
    return *Singleton<NGeobase::TLookup>(JoinFsPaths(GetWorkPath(), "geodata6/geodata6.bin"));
}

// TRequestJson ---------------------------------------------------------------
TRequestJson::TRequestJson()
    : Request{NSc::TValue::FromJsonThrow(DEFAULT_REQUEST_JSON_META)}
{
    SetForm(NBASS::TUnitTestFormHandler::DEFAULT_FORM_NAME);
}

TRequestJson::TRequestJson(TStringBuf jsonString)
    : Request{NSc::TValue::FromJsonThrow(jsonString)}
{
}

TRequestJson& TRequestJson::SetClient(TStringBuf client) {
    // TODO also patch meta/client
    Request["meta"]["client_id"].SetString(client);
    return *this;
}

TRequestJson& TRequestJson::SetExpFlag(TStringBuf expFlag, TStringBuf value) {
    Request["meta"]["experiments"][expFlag].SetString(value);
    return *this;
}

TRequestJson& TRequestJson::SetForm(TStringBuf formName) {
    NSc::TValue& formJson = Request["form"];
    formJson["name"].SetString(formName);
    formJson["slots"].SetArray();
    return *this;
}

TRequestJson& TRequestJson::SetUID(ui64 uid) {
    Request["meta"]["uid"].SetIntNumber(uid);
    return *this;
}

} // namespace NTestingHelpers
