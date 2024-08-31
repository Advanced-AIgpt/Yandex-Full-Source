#include "watchdog.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/config/config.h>
#include <alice/megamind/library/watchdog/watchdog.h>

#include <alice/library/metrics/names.h>

#include <library/cpp/watchdog/watchdog.h>

#include <util/datetime/base.h>
#include <util/random/random.h>

#include <functional>


namespace NAlice::NMegamind {
namespace {

struct TInfo {
    TInfo(NAppHost::IServiceContext& ctx)
        : Path{ctx.GetLocation().Path}
        , Ruid{ctx.GetRUID()}
    {
        if (const auto* appHostParams = ctx.FindFirstItem(AH_ITEM_APPHOST_PARAMS)) {
            ReqId = (*appHostParams)["reqid"].GetString();
        }
    }

    TString AsString() const {
        return TStringBuilder{} << "ReqId: '" << ReqId << "', RUID: '"
                                << Ruid << "', NodePath: '" << Path << '\'';
    }

    TString Path;
    TString ReqId;
    ui64 Ruid;
};

class TTimeouts {
public:
    TTimeouts(TDuration timeout)
        : AbortTimeout_{timeout + TDuration::MilliSeconds(RandomNumber(timeout.MilliSeconds() * 30))}
    {
    }

    TDuration AbortTimeout() const {
        return AbortTimeout_;
    }

    TDuration WarnTimeout() const {
        static constexpr auto fifteenSeconds = TDuration::Seconds(15);
        static constexpr double multiplier = 0.7;
        return Min<TDuration>(AbortTimeout_ * multiplier, AbortTimeout_ - fifteenSeconds);
    }

private:
    TDuration AbortTimeout_;
};

std::function<void()> ConstructWarnCallback(const TInfo& info, IGlobalCtx& globalCtx) {
    return [info, &sensors = globalCtx.ServiceSensors()]() {
        Cerr << "WatchDog is going to kill node soon: " << info.AsString() << Endl;
        NMonitoring::TLabels labels{
            {"name", NSignal::WATCHDOG_REQUEST_KILL_SOON},
            {"path", info.Path},
        };
        sensors.AddRate(labels, 1);
    };
}

TWatchDogHolder ConstructWatchDog(const TInfo& info, IGlobalCtx& globalCtx) {
    const TTimeouts timeouts{TDuration::Seconds(globalCtx.Config().GetWatchDogRequestTimeoutSeconds())};

    TWatchDogHolder watchDog;
    watchDog.Add(TWatchDogPtr(CreateAbortByTimeoutWatchDog(TTimeoutWatchDogOptions(timeouts.AbortTimeout()), info.AsString())))
            .Add(TWatchDogPtr(CreateTimeoutWatchDog(timeouts.WarnTimeout(), ConstructWarnCallback(info, globalCtx))));
    return watchDog;
}

} // namespace

TWatchDogRegistry::TWatchDogRegistry(TAppHostDispatcher* appHostDispatcher, IGlobalCtx& globalCtx)
    : TRegistry{appHostDispatcher}
    , GlobalCtx_{globalCtx}
{
}

void TWatchDogRegistry::Add(const TString& path, NAppHost::TAsyncAppService handler) {
    auto& globalCtx = GlobalCtx_;
    TRegistry::Add(path, [handler, &globalCtx](NAppHost::TServiceContextPtr ctx) {
        const TInfo info{*ctx};
        const auto watchDog = ConstructWatchDog(info, globalCtx);
        return handler(ctx);
    });
}

void TWatchDogRegistry::Add(const TString& path, NAppHost::TAppService handler) {
    auto& globalCtx = GlobalCtx_;
    auto onRequest = [handler, &globalCtx](NAppHost::IServiceContext& ctx) {
        const TInfo info{ctx};
        const auto watchDog = ConstructWatchDog(info, globalCtx);
        return handler(ctx);
    };
    TRegistry::Add(path, onRequest);
}

void TWatchDogRegistry::Add(const TString& path, NNeh::TServiceFunction handler) {
    const TTimeouts timeouts{TDuration::Seconds(GlobalCtx_.Config().GetWatchDogRequestTimeoutSeconds())};
    auto onRequest = [handler, path, timeout=timeouts.AbortTimeout()](const NNeh::IRequestRef& req) {
        TWatchDogHolder watchDog;
        watchDog.Add(TWatchDogPtr(CreateAbortByTimeoutWatchDog(TTimeoutWatchDogOptions(timeout), path)));
        return handler(req);
    };
    TRegistry::Add(path, onRequest);
}

} // namespace NAlice::NMegamind
