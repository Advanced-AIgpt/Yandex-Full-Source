#include "node.h"
#include "stage_timers.h"
#include "util.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/requestctx/requestctx.h>
#include <alice/megamind/library/requestctx/rtlogtoken.h>
#include <alice/megamind/library/response_meta/error.h>

#include <alice/library/network/common.h>
#include <alice/library/metrics/histogram.h>
#include <alice/library/metrics/names.h>

#include <apphost/lib/common/constants.h>
#include <apphost/lib/proto_answers/http.pb.h>

#include <alice/megamind/library/apphost_request/protos/uniproxy_request.pb.h>
#include <alice/megamind/library/apphost_request/protos/required_node_meta.pb.h>

#include <util/generic/yexception.h>

namespace NAlice::NMegamind {
namespace {
TRequestCtx::IInitializer::TStageTimersPtr ConstructStageTimers(IAppHostCtx& ahCtx) {
    auto stageTimers = std::make_unique<TStageTimersAppHost>(ahCtx.ItemProxyAdapter());
    stageTimers->LoadFromContext(ahCtx);
    return std::move(stageTimers);
}

TAppHostRequestCtx::TInitializer ConstructInitializer(IAppHostCtx& ahCtx) {
    NMegamindAppHost::TUniproxyRequestInfoProto upProto;
    if (const auto err = GetFirstProtoItem(ahCtx.ItemProxyAdapter(), AH_ITEM_UNIPROXY_REQUEST, upProto)) {
        ythrow yexception() << *err;
    }

    NUri::TUri uri;
    uri.ParseUri(upProto.GetUri(), NUri::TFeature::FeaturesDefault | NUri::TFeature::FeatureSchemeFlexible);

    TCgiParameters cgi;
    cgi.ScanAdd(upProto.GetCgi());

    THttpHeaders headers;
    for (const auto& header : upProto.GetHeaders()) {
        headers.AddHeader(header.GetName(), header.GetValue());
    }

    return TAppHostRequestCtx::TInitializer{ahCtx, std::move(uri), std::move(cgi), std::move(headers)};
}

NMonitoring::TLabels CreateExecutionTimeSignal(NAppHost::IServiceContext& ctx) {
    return NMonitoring::TLabels{
        {"name", NSignal::APPHOST_NODE_EXECUTION_TIME},
        {"path", ctx.GetLocation().Path},
        {"node", ctx.GetLocation().Name}
    };
}

TStatus GetRequiredNodeMeta(NAppHost::IServiceContext& ctx, NMegamindAppHost::TRequiredNodeMeta& nodeMeta) {
    const auto& items = ctx.GetProtobufItemRefs(AH_ITEM_REQUIRED_NODE_META);
    if (items.empty()) {
        return TError{TError::EType::NotFound} << "failed to find " << AH_ITEM_REQUIRED_NODE_META << " item";
    }

    if (!items.front().Fill(&nodeMeta)) {
        return TError{TError::EType::Parse} << "failed to parse protobuf from " << AH_ITEM_REQUIRED_NODE_META << " item";
    }

    return Success();
}

class TAppHostCtx : public IAppHostCtx {
public:
    TAppHostCtx(NAppHost::IServiceContext& ctx, TRTLogger& log, IGlobalCtx& globalCtx, bool useAppHostStreaming)
        : Ctx_{ctx}
        , Log_{log}
        , GlobalCtx_{globalCtx}
        , ItemProxyAdapter_{Ctx_, Log_, GlobalCtx_, useAppHostStreaming}
    {
    }

    TRTLogger& Log() override {
        return Log_;
    }

    IGlobalCtx& GlobalCtx() override {
        return GlobalCtx_;
    }

    TItemProxyAdapter& ItemProxyAdapter() override {
        return ItemProxyAdapter_;
    }

private:
    NAppHost::IServiceContext& Ctx_;
    TRTLogger& Log_;
    IGlobalCtx& GlobalCtx_;
    TItemProxyAdapter ItemProxyAdapter_;
};

} // namespace

// TAppHostNodeHandler ---------------------------------------------------------
void TAppHostNodeHandler::RunSync(NAppHost::IServiceContext& ctx) const {
    THistogramScope runScope{GlobalCtx.ServiceSensors(),
                             CreateExecutionTimeSignal(ctx),
                             THistogramScope::ETimeUnit::Millis};

    auto logger = CreateLogger(ctx);

    const auto& loc = ctx.GetLocation();
    LOG_INFO(logger) << "AppHost node: '" << loc.Path << '\'';

    auto log = [&logger, &loc](TStringBuf msg) {
        LOG_ERR(logger) << "AppHost node '" << loc.Path << "' error: " << msg;
    };

    // TODO (petrk) Get rid of TRequestCtx::TBadRequestException exception, use status instead.
    // (https://st.yandex-team.ru/MEGAMIND-1995)
    try {
        auto ahCtx = CreateContext(ctx, logger);
        if (const auto err = Execute(*ahCtx)) {
            log(err->ErrorMsg);
            PushErrorItem(ctx, TErrorMetaBuilder{*err});
        }
    } catch (const TRequestCtx::TBadRequestException& e) {
        log(e.what());
        PushErrorItem(ctx, TErrorMetaBuilder{TError{TError::EType::BadRequest} << e.what()});
    } catch (...) {
        const TString msg = TBackTrace::FromCurrentException().PrintToString();
        log(msg);
        PushErrorItem(ctx, TErrorMetaBuilder{TError{TError::EType::Exception} << msg});
    }
}

NThreading::TFuture<void> TAppHostNodeHandler::RunAsync(NAppHost::TServiceContextPtr ctx) const {
    try {
        RunSync(*ctx);
    } catch (...) {
        TBackTrace::FromCurrentException().PrintTo(Cerr);
    }
    return NThreading::MakeFuture();
}

std::unique_ptr<IAppHostCtx> TAppHostNodeHandler::CreateContext(NAppHost::IServiceContext& ctx, TRTLogger& log) const {
    return std::make_unique<TAppHostCtx>(ctx, log, GlobalCtx, UseAppHostStreaming());
}

TRTLogger TAppHostNodeHandler::CreateLogger(NAppHost::IServiceContext& ctx) const {
    TRTLoggerTokenConstructor rtLogToken{ToString(ctx.GetRUID())};
    if (const auto* appHostParams = ctx.FindFirstItem(AH_ITEM_APPHOST_PARAMS)) {
        rtLogToken.SetAppHostReqId((*appHostParams)["reqid"].GetString());
    }

    TMaybe<ELogPriority> logPriorityFromRequest;
    {
        const auto items = ctx.GetProtobufItemRefs(AH_ITEM_LOGGER_OPTIONS);
        NAlice::TLoggerOptions loggerOptions;
        if (!items.empty() && items.front().Fill(&loggerOptions)) {
            const auto logLevel = MapUniproxyLogLevelToMegamindLogLevel(loggerOptions.GetSetraceLogLevel());
            if (logLevel.IsSuccess()) {
                logPriorityFromRequest = logLevel.Value();
            }
        }
    }

    auto log = GlobalCtx.RTLogger(rtLogToken.GetToken(), /* session= */ false, logPriorityFromRequest);
    NMegamindAppHost::TRequiredNodeMeta nodeMeta;

    if (const auto err = GetRequiredNodeMeta(ctx, nodeMeta)) {
        LOG_WARN(log) << "Unable to update logger request id: " << err;
    } else {
        UpdateLoggerRequestId(ToString(ctx.GetLocation().Path), nodeMeta, log);
    }
    return log;
}

// static
void TAppHostNodeHandler::UpdateLoggerRequestId(const TString& nodeLocation,
                                                const NMegamindAppHost::TRequiredNodeMeta& nodeMeta,
                                                TRTLogger& log)
{
    log.UpdateRequestId(nodeMeta.GetSpeechKitRequestId(),
                        nodeMeta.GetHypothesisNumber(),
                        nodeMeta.GetEndOfUtterance(),
                        nodeLocation);
}

// TAppHostRequestCtx::TInitializer -------------------------------------------
TAppHostRequestCtx::TInitializer::TInitializer(IAppHostCtx& ahCtx, NUri::TUri uri, TCgiParameters cgi, THttpHeaders headers)
    : TInitializer{ahCtx, uri, cgi, headers, ConstructStageTimers(ahCtx)}
{
}

TAppHostRequestCtx::TInitializer::TInitializer(IAppHostCtx& ahCtx, NUri::TUri uri, TCgiParameters cgi, THttpHeaders headers, TStageTimersPtr stageTimers)
    : AhCtx_{ahCtx}
    , Uri_{std::move(uri)}
    , Cgi_{std::move(cgi)}
    , Headers_{std::move(headers)}
    , StageTimers_{std::move(stageTimers)}
{
}

// TAppHostRequestCtx ---------------------------------------------------------
TAppHostRequestCtx::TAppHostRequestCtx(IAppHostCtx& ahCtx)
    : TRequestCtx{ahCtx.GlobalCtx(), ConstructInitializer(ahCtx)}
    , ItemProxyAdapter_{ahCtx.ItemProxyAdapter()}
{
}

} // namespace NAlice::NMegamind
