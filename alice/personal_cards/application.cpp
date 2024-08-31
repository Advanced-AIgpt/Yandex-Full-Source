#include "application.h"

#include "http_request.h"
#include "server.h"

#include <alice/bass/libs/logging/logger.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/proto_config/load.h>
#include <library/cpp/sighandler/async_signals_handler.h>

#include <util/stream/file.h>
#include <util/system/mlock.h>

namespace NPersonalCards {

TApplication* TApplication::Instance_ = nullptr;

TApplication::TApplication(TConfig&& config)
    : Config_(config)
    , MtpQueue_(new TThreadPool())
    , IsShutdownInitiated_(false)
    , PushCardsStorage_(CreateYDBPushCardsStorage(Config_.GetYDBClient()))
    , ParallelHttpRequestCount_(0)
    , MaxParallelHttpRequestCountInWindow_(0)
    , TvmClient_(nullptr)
    , TrustedServices_({
        Config_.GetTvm().GetTrustedServicesTvmIds().begin(),
        Config_.GetTvm().GetTrustedServicesTvmIds().end()
    })
    , RequestTimeLimit_(FromString<TDuration>(config.GetRequestTimeLimit()))
{
    Y_ASSERT(!Instance_);
    Instance_ = this;

    InitSensors();

    THttpServer::TOptions options(Config_.GetHttpServer().GetPort());
    options.SetThreads(Config_.GetHttpServer().GetThreads());
    options.EnableKeepAlive(true);
    options.EnableCompression(true);
    HttpServer_.Reset(new THttpServer(this, options));

    static NInfra::TIntGaugeSensor mlockSensor(SENSOR_GROUP, "memory_lock_is_off");
    if (Config_.GetLockAllMemory()) {
        try {
            LockAllMemory(ELockAllMemoryFlag::LockCurrentMemory | ELockAllMemoryFlag::LockFutureMemory);
            mlockSensor.Set(0);
        } catch (...) {
            mlockSensor.Set(1);
            LOG(ERR) << "Failed to lock memory: " << CurrentExceptionMessage() << Endl;
        }
    } else {
        mlockSensor.Set(1);
        LOG(WARNING) << "Memory lock is off!" << Endl;
    }

    try {
        TvmClient_ = CreateTvmClient(config.GetTvm(), true /* withRetries */);
    } catch (...) {
        LOG(ERR) << "Unable to construct tvm client: " << CurrentExceptionMessage() << Endl;
    }
}

TApplication::~TApplication() {
    // Remove signal handlers to don't use TApplication after it was deleted.
    SetAsyncSignalFunction(SIGTERM, nullptr);
    SetAsyncSignalFunction(SIGINT, nullptr);

    MtpQueue_ = nullptr;
    if (Instance_ == this) {
        Instance_ =  nullptr;
    }
}

// static
TApplication* TApplication::Init(int argc, const char** argv) {
    auto config = NProtoConfig::GetOpt<TConfig>(argc, argv, "/proto_config/config.json");
    TLogging::Configure(config.GetLogger().GetDir(), config.GetHttpServer().GetPort(), FromString<ELogPriority>(config.GetLogger().GetLevel()));
    static TApplication app(std::move(config));

    // Remove these signal handlers in d-tor.
    auto shutdown = [](int) {
        TApplication::GetInstance()->Shutdown();
    };
    SetAsyncSignalFunction(SIGTERM, shutdown);
    SetAsyncSignalFunction(SIGINT, shutdown);

    return &app;
}

int TApplication::Run() {
    MtpQueue_->Start(Config_.GetHttpServer().GetThreads());

    CardsServer_ = MakeHolder<TServer>(PushCardsStorage_);

    while (!HttpServer_->Start()) {
        if (IsShutdownInitiated_.load(std::memory_order_acquire)) {
            return 0;
        }
        LOG(INFO) << "starting HTTP server..." << Endl;
        Sleep(TDuration::Seconds(1));
    }
    LOG(INFO) << "HTTP server started on port " << Config_.GetHttpServer().GetPort() << Endl;

    HttpServer_->Wait();
    LOG(INFO) << "HTTP server stopped" << Endl;

    return 0;
}

void TApplication::Shutdown() {
    IsShutdownInitiated_.store(true, std::memory_order_release);

    if (CardsServer_) {
        CardsServer_->Shutdown();
    }

    LOG(INFO) << "HTTP server shutdown initiated" << Endl;
    HttpServer_->Shutdown();
}

void TApplication::OnMaxConn() {
    static NInfra::TRateSensor rateSensor(SENSOR_GROUP, "server_max_conn");
    rateSensor.Inc();
}

TClientRequest* TApplication::CreateClient() {
    return new THttpReplier;
}

void TApplication::InitSensors() {
    {
        NInfra::TRateSensor(SENSOR_GROUP, "add_push_card", {{
            "error", "proto_parse"
        }}).Add(0);
    }
    {
        NInfra::TRateSensor(SENSOR_GROUP, "dismiss_card", {{
            "error", "proto_parse"
        }}).Add(0);
    }
    {
        NInfra::TRateSensor(SENSOR_GROUP, "add_push_card", {{
            "error", "more_than_one_user_id"
        }}).Add(0);
    }
    {
        NInfra::TRateSensor(SENSOR_GROUP, "dismiss_card" ,{{
            "error", "more_than_one_user_id"
        }}).Add(0);
    }
    for (const TString& type : {"uid", "did", "device_id", "uuid"}) {
        NInfra::TRateSensor(SENSOR_GROUP, "id_in_request" ,{{
            "type", type
        }}).Add(0);
    }

    static const TVector<TString> gaugeSensors = {
        "server_max_conn"
        "active_ydb_session_count",
        "parallel_http_request_count",
    };
    static const TVector<TString> rateSensors = {
        "http_request_badjson",
        "http_request_notfound",
        "http_request_exception",
        "push_cards_storage_get_cards_empty_responses"
    };
    static const TVector<TString> routes = {
        "/addPushCard",
        "/addPushCards",
        "/dismiss",
        "/removePushCard",
        "/cards",
        "/getPushCards",
        // All routes below are deprected.
        "/addCards",
        "/personalCards",
        "/updateCards",
        "/userContext",
        "/loadCards",
        "/updateCardsData",
        "/userInfo",
        "/getStoriesInfo",
        "/addStoriesWatch"
    };

    for (const auto& sensor : rateSensors) {
        NInfra::TRateSensor(SENSOR_GROUP, sensor).Add(0);
    }
    for (const auto& sensor : gaugeSensors) {
        NInfra::TIntGaugeSensor(SENSOR_GROUP, sensor).Add(0);
    }

    for (const auto& route : routes) {
        // Http codes
        for (int i = 1; i <= 5; ++i) {
            NInfra::TRateSensor(SENSOR_GROUP, "request_result", {
                {"code", TStringBuilder() << i << "XX"},
                {"route", route}
            }).Add(0);
        }

        // Time limit
        NInfra::TRateSensor(SENSOR_GROUP, "request_time_limit_exceeded", {{"route", route}}).Add(0);

        // Http request count and time
        NInfra::TRateSensor(SENSOR_GROUP, "http_request_count", {{"route", route}}).Add(0);
        THistogramTimer("http_request_time", {{"route", route}});
    }

    // Push cards storage
    for (const auto& opName : {"add_push_card", "dismiss_push_card", "get_push_card"}) {
        NInfra::TRateSensor(SENSOR_GROUP, "push_cards_storage_fails", {{"operation", opName}}).Add(0);
    }
}

} // namespace NPersonalCards
