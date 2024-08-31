#include "apphost_dispatcher.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/response.h>
#include <alice/megamind/library/config/config.h>
#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/util/status.h>

#include <alice/library/logger/logger.h>
#include <alice/library/metrics/gauge.h>
#include <alice/library/metrics/names.h>

#include <apphost/api/service/cpp/service_exceptions.h>

#include <library/cpp/http/server/response.h>
#include <library/cpp/threading/future/async.h>
#include <library/cpp/threading/future/future.h>

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/strbuf.h>

#include <utility>

namespace NAlice::NMegamind {
namespace {

template <typename T, typename F>
T CheckedCast(F src, TStringBuf msg) {
    Y_ENSURE(src > Min<T>() && src < Max<T>(), msg);
    return static_cast<T>(src);
}

template <typename T, typename F>
T CheckedCast(F base, F offset, TStringBuf msg) {
    const F result = base + offset;
    Y_ENSURE(result > base, msg);
    return CheckedCast<T>(result, msg);
}

} // namespace

TAppHostDispatcher::TPort::TPort(const TConfig& config) {
    const auto& appHostConfig = config.GetAppHost();

    Http = CheckedCast<ui16>(appHostConfig.GetHttpPort(), "Invalid apphost http port value");
    if (appHostConfig.HasGrpcPort()) {
        Grpc = CheckedCast<ui16>(appHostConfig.GetGrpcPort(), "Invalid apphost grpc port value");
    } else {
        Grpc = CheckedCast<ui16>(appHostConfig.GetHttpPort(), appHostConfig.GetGrpcPortOffset(), "Invalid apphost grpc port offset value");
    }
}

TAppHostDispatcher::TAppHostDispatcher(IGlobalCtx& globalCtx)
    : Config_{globalCtx.Config()}
    , Port_{Config_}
    , Log_{globalCtx.BaseLogger()}
    , Loop_{NAppHost::TLoopParams{.UseGrpcServiceV2 = globalCtx.Config().GetAppHost().GetUseGrpcServiceV2()}}
    , Sensors_{globalCtx.ServiceSensors()}
{
    if (const auto numThreads = Config_.GetAppHost().GetAsyncWorkerThreads(); numThreads > 0) {
        LOG_INFO(Log_) << "Starting thread pool for apphost async requests: " << numThreads;
        AsyncThreadPool_ = CreateThreadPool(numThreads, 0, TThreadPoolParams{}.SetThreadNamePrefix("apphost_async_thread_"));
        if (Config_.GetAppHost().GetUseMaxInProgressLimit()) {
            LOG_INFO(Log_) << "Enable apphost limit threads in progress to: " << (numThreads - 1);
            Loop_.SetMaxRequestsInProcess(numThreads - 1);
        }
    }
}

void TAppHostDispatcher::Add(const TString& path, NAppHost::TAsyncAppService handler) {
    if (AsyncThreadPool_) {
        LOG_INFO(Log_) << "Registered async apphost handler in the separate thread pool '" << path << "' @ " << Port_.Http;
        NAppHost::TAsyncAppService newHandler = [handler, this](NAppHost::TServiceContextPtr ctx) {
            try {
                return NThreading::Async([ctx, handler, this]() {
                        try {
                            TOngoingRequestsCounter counter{Sensors_, NSignal::APPHOST_ASYNC_THREADPOOL_ONGOING};
                            const auto result = handler(ctx);
                            return result.GetValueSync();
                        } catch (...) {
                            auto labels = NSignal::APPHOST_REQUEST_EXCEPTION;
                            labels.Add("htype", "separate_async");
                            Sensors_.AddRate(labels, 1);
                        }
                    }, *AsyncThreadPool_);
            } catch (const TThreadPoolException& e) {
                return NThreading::MakeErrorFuture<void>(std::make_exception_ptr(NAppHost::NService::TFastError()));
            }
        };
        Loop_.Add(Port_.Http, path, newHandler);
    } else {
        LOG_INFO(Log_) << "Registered async apphost handler in the apphost thread pool '" << path << "' @ " << Port_.Http;
        Loop_.Add(Port_.Http, path, handler);
    }
}

void TAppHostDispatcher::Add(const TString& path, NAppHost::TAppService handler) {
    LOG_INFO(Log_) << "Registered sync apphost handler '" << path << "' @ " << Port_.Http;
    Loop_.Add(Port_.Http, path, handler);
}

void TAppHostDispatcher::Add(const TString& path, NNeh::TServiceFunction handler) {
    LOG_INFO(Log_) << "Registered http apphost handler '" << path << "' @ " << Port_.Http;
    Loop_.Add(Port_.Http, path, handler);
}

void TAppHostDispatcher::Start() {
    LOG_INFO(Log_) << "Trying to start AppHost loop";

    const auto& appHostConfig = Config_.GetAppHost();

    Loop_.EnableGrpc(Port_.Grpc,
                     true /* reusePort */,
                     TDuration::MilliSeconds(appHostConfig.GetStreamingSessionTimeoutMs()));
    Loop_.SetGrpcThreadCount(CheckedCast<size_t>(appHostConfig.GetGrpcTransportThreads(), "Invalid apphost grpc transport threads value"));
    Loop_.ForkLoop(appHostConfig.GetWorkerThreads());
    LOG_INFO(Log_) << "AppHost http (" << Port_.Http << ") and grpc (" << Port_.Grpc << ") is started";
}

void TAppHostDispatcher::Stop() {
    LOG_INFO(Log_) << "Trying to stop AppHost loop";
    Loop_.SyncStopFork();
    LOG_INFO(Log_) << "AppHost loop has been stopped";

    if (AsyncThreadPool_) {
        LOG_INFO(Log_) << "Starting to stop threadpool for AppHost async requests (active tasks: " << AsyncThreadPool_->Size() << ')';
        AsyncThreadPool_->Stop();
        LOG_INFO(Log_) << "Threadpool for AppHost async requests has stopped";
    }
}

} // namespace NAlice::NMegamind
