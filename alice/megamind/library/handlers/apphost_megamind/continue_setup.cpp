#include "continue_setup.h"

#include "blackbox.h"
#include "components.h"
#include "responses.h"

#include <alice/megamind/library/apphost_request/request_builder.h>
#include <alice/megamind/library/apphost_request/protos/scenario.pb.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/request/request.h>
#include <alice/megamind/library/request/internal/protos/request_data.pb.h>
#include <alice/megamind/library/scenarios/protocol/helpers.h>
#include <alice/megamind/library/speechkit/request.h>

#include <alice/megamind/protos/scenarios/request.pb.h>

#include <alice/library/scenarios/utils/utils.h>


namespace NAlice::NMegamind {

namespace {

class TContinueSourceResponses final : public TAppHostSourceResponses {
public:
    explicit TContinueSourceResponses(IAppHostCtx& ahCtx)
        : AhCtx_{ahCtx}
    {
    }

private:
    void InitBlackBoxResponse() const override {
        BlackBoxResponse_->Status = AppHostBlackBoxPostSetup(AhCtx_, BlackBoxResponse_->Object);
    }

private:
    IAppHostCtx& AhCtx_;
};

struct TScenarioItemInfo {
    TScenarioItemInfo(const TString& name, bool isPure)
        : Name{name}
        , IsPure{isPure}
        , ItemNames{Name,
                    isPure ? PURE_REQUEST_SUFFIX : HTTP_PROXY_REQUEST_SUFFIX,
                    isPure ? PURE_RESPONSE_SUFFIX : HTTP_PROXY_RESPONSE_SUFFIX}
    {
    }

    TString Name;
    bool IsPure;
    TAppHostItemNames ItemNames;
};

TMaybe<TScenarioItemInfo> TryParseItemNameAsScenarioResponse(TStringBuf name) {
    if (!name.SkipPrefix("scenario_")) {
        return Nothing();
    }
    bool isPure = name.ChopSuffix("_run_pure_response");
    bool isProxy = name.ChopSuffix("_run_http_proxy_response");
    if (!isPure && !isProxy) {
        return Nothing();
    }
    return TScenarioItemInfo{TString{name}, isPure};
}

TErrorOr<NScenarios::TScenarioRunResponse> TryGetRunResponseFromContext(IAppHostCtx& ahCtx, const TScenarioItemInfo& itemInfo) {
    const auto itemName = itemInfo.ItemNames.RunResponse;
    if (itemInfo.IsPure) {
        return ahCtx.ItemProxyAdapter().GetFromContext<NScenarios::TScenarioRunResponse>(itemName);
    } else {
        TErrorOr<NAppHostHttp::THttpResponse> httpResponseProto =
                ahCtx.ItemProxyAdapter().GetFromContext<NAppHostHttp::THttpResponse>(itemName);
        return ParseScenarioResponse<NScenarios::TScenarioRunResponse>(httpResponseProto, "Run");
    }
}

void ProcessScenarioResponse(IAppHostCtx& ahCtx, const TContext& ctx,
                             const TScenarioItemInfo& itemInfo) {
    const auto item = TryGetRunResponseFromContext(ahCtx, itemInfo);
    if (item.Error() || !item.Value().HasContinueArguments()) {
        return;
    }
    auto args = item.Value().GetContinueArguments();

    const auto itemBaseRequest =
        ahCtx.ItemProxyAdapter().GetFromContext<NScenarios::TScenarioBaseRequest>(itemInfo.ItemNames.BaseRequest);
    const auto itemInput =
        ahCtx.ItemProxyAdapter().GetFromContext<NScenarios::TInput>(itemInfo.ItemNames.RequestInput);
    if (itemBaseRequest.Error() || itemInput.Error()) {
        LOG_ERR(ctx.Logger()) << "Cannot get requered items from context to create "
                                << itemInfo.Name << " scenario continue request";
    }

    NScenarios::TScenarioApplyRequest continueRequest;
    continueRequest.MutableBaseRequest()->CopyFrom(itemBaseRequest.Value());
    continueRequest.MutableInput()->CopyFrom(itemInput.Value());
    continueRequest.MutableArguments()->CopyFrom(args);

    if (itemInfo.IsPure) {
        const auto itemRequestMeta =
            ahCtx.ItemProxyAdapter().GetFromContext<NMegamindAppHost::TRequestMeta>(itemInfo.ItemNames.RequestMeta);
        if (itemRequestMeta.Error()) {
            LOG_ERR(ctx.Logger()) << "Cannot get request_meta from context to create "
                                    << itemInfo.Name << " scenario continue request";
        }

        ahCtx.ItemProxyAdapter().PutIntoContext(continueRequest, itemInfo.ItemNames.ContinueRequest);
        ahCtx.ItemProxyAdapter().PutIntoContext(itemRequestMeta.Value(), itemInfo.ItemNames.RequestMeta);
    } else {
        const auto& config = ahCtx.GlobalCtx().ScenarioConfigRegistry().GetScenarioConfig(itemInfo.Name);
        NMegamind::TAppHostHttpProxyMegamindRequestBuilder builder;
        TStatus error = FillHttpProxyRequest(ctx, continueRequest, builder,
                                             ctx.IsOAuthEnabled(config.GetName()));
        if (error.Defined()) {
            LOG_ERR(ctx.Logger()) << "Cannot create " << itemInfo.Name << " scenario continue request: " << *error;
        }
        builder.SetPath("/continue");
        ahCtx.ItemProxyAdapter().PutIntoContext(builder.CreateRequest(), itemInfo.ItemNames.ContinueRequest);
    }

    ahCtx.ItemProxyAdapter().IntermediateFlush();
};

} // namespace

TStatus TContinueSetupNodeHandler::Execute(IAppHostCtx& ahCtx) const {
    TFromAppHostSpeechKitRequest::TPtr skr;
    if (auto err = TFromAppHostSpeechKitRequest::Create(ahCtx).MoveTo(skr)) {
        return err;
    }
    TSpeechKitRequest skrView = *skr;
    TAppHostRequestCtx requestCtx{ahCtx};
    TContext ctx{skrView, MakeHolder<TContinueSourceResponses>(ahCtx), requestCtx};

    THashSet<TString> scenarioNames;
    auto onItem = [&scenarioNames](NMegamindAppHost::TScenarioProto& proto) {
        scenarioNames.insert(proto.GetName());
    };
    ahCtx.ItemProxyAdapter().ForEachCached<NMegamindAppHost::TScenarioProto>(AH_ITEM_SCENARIO, onItem);

    while (!scenarioNames.empty()) {
        for (const auto& item : ahCtx.ItemProxyAdapter().GetItemNamesFromCache()) {
            const auto scenarioItemInfo = TryParseItemNameAsScenarioResponse(item);
            if (!scenarioItemInfo.Defined() || !scenarioNames.contains(scenarioItemInfo->Name)) {
                continue;
            }
            scenarioNames.erase(scenarioItemInfo->Name);
            ProcessScenarioResponse(ahCtx, ctx, *scenarioItemInfo);
        }
        if (!ahCtx.ItemProxyAdapter().WaitNextInput()) {
            break;
        }
    }

    return Success();
}

void RegisterContinueSetupHandler(IGlobalCtx& globalCtx, TRegistry& registry) {
    static const TContinueSetupNodeHandler continueSetupHandler{globalCtx, /* useAppHostStreaming= */ true};
    registry.Add("/mm_continue_setup", [](NAppHost::IServiceContext& ctx) { continueSetupHandler.RunSync(ctx); });
}

} // namespace NAlice::NMegaamind
