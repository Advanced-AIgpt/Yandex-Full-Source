#include "fallback_response.h"
#include "stage_timers.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/protos/uniproxy_request.pb.h>
#include <alice/megamind/library/apphost_request/response.h>
#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/handlers/utils/sensors.h>
#include <alice/megamind/library/response_meta/error.h>

#include <alice/library/json/json.h>
#include <alice/library/metrics/sensors.h>
#include <alice/library/metrics/util.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/yexception.h>
#include <util/string/builder.h>

namespace NAlice::NMegamind {
namespace {

void UpdateMetricsData(TRequestTimeMetrics& rtm, IAppHostCtx& ahCtx) {
    {
        auto stageTimers = std::make_unique<TStageTimersAppHost>(ahCtx.ItemProxyAdapter());
        stageTimers->LoadFromContext(ahCtx);

        if (const auto* startTime = stageTimers->Find(TS_STAGE_START_REQUEST); startTime != nullptr) {
            rtm.SetStartTime(*startTime);
        }
    }

    NMegamindAppHost::TUniproxyRequestInfoProto upProto;
    if (GetFirstProtoItem(ahCtx.ItemProxyAdapter(), AH_ITEM_UNIPROXY_REQUEST, upProto)) {
        rtm.SetSkrInfo("unknown", "unknown");
    } else {
        rtm.SetSkrInfo(upProto.GetClientName(), upProto.GetUri());
    }
}

} // namespace

TAppHostFallbackResponseNodeHandler::TAppHostFallbackResponseNodeHandler(IGlobalCtx& globalCtx)
    : TAppHostNodeHandler{globalCtx, /* useAppHostStreaming= */ false}
{
}

TStatus TAppHostFallbackResponseNodeHandler::Execute(IAppHostCtx& ahCtx) const {
    auto& log = ahCtx.Log();
    TMaybe<TErrorMetaBuilder> errorMetaBuilder;

    TRequestTimeMetrics requestTimeMetrics{ahCtx.GlobalCtx().ServiceSensors(), log,
                                           ahCtx.ItemProxyAdapter().NodeLocation().Name,
                                           EUniproxyStage::Run, ERequestResult::Fail};

    UpdateMetricsData(requestTimeMetrics, ahCtx);

    try {
        TVector<TErrorMetaBuilder::TMetaProto> errorItems;
        auto callback = [&errorItems](const TErrorMetaBuilder::TMetaProto& proto) {
            errorItems.push_back(proto);
        };
        ahCtx.ItemProxyAdapter().ForEachCached<TErrorMetaBuilder::TMetaProto>(AH_ITEM_ERROR, callback);
        if (errorItems.empty()) {
            errorMetaBuilder.ConstructInPlace(TError{TError::EType::System} << "No mm_error item found (must not happen)");
            errorMetaBuilder->SetNetLocation(ahCtx.ItemProxyAdapter().NodeLocation().Path);
        } else {
            errorMetaBuilder.ConstructInPlace(std::move(*errorItems.rbegin()));
        }

        for (auto& errorItem : errorItems) {
            if (&errorItem == &errorItems.back()) {
                break;
            }

            LOG_ERR(log) << "Got an error during request processing: " << errorItem.ShortUtf8DebugString();
            errorMetaBuilder->AppendNested(std::move(errorItem));
        }
    } catch (...) {
        const TString msg = TString::Join("Exception during error meta response creation: ", CurrentExceptionMessage());
        LOG_ERR(log) << msg;
        if (!errorMetaBuilder.Defined()) {
            errorMetaBuilder.ConstructInPlace(TError{TError::EType::Exception} << msg);
        }
    }

    TAppHostHttpResponse httpResponse{ahCtx.ItemProxyAdapter()};
    errorMetaBuilder->ToHttpResponse(httpResponse);

    return Success();
}

// static
void TAppHostFallbackResponseNodeHandler::Register(IGlobalCtx& globalCtx, TRegistry& registry) {
    static const TAppHostFallbackResponseNodeHandler handler{globalCtx};
    registry.Add("/mm_fallback_response", [](NAppHost::IServiceContext& ctx) { handler.RunSync(ctx); });
}

} // namespace NAlice::NMegamind
