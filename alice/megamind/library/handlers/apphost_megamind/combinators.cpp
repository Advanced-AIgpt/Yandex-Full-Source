#include "combinators.h"

#include "begemot.h"
#include "components.h"

#include <alice/megamind/library/apphost_request/item_adapter.h>
#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/protos/combinators.pb.h>
#include <alice/megamind/library/begemot/begemot.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/memento/memento.h>
#include <alice/megamind/library/request/request.h>
#include <alice/megamind/library/scenarios/protocol/protocol_scenario.h>
#include <alice/megamind/library/speechkit/request.h>

#include <alice/megamind/protos/common/location.pb.h>
#include <alice/megamind/protos/scenarios/combinator_request.pb.h>

#include <alice/begemot/lib/api/experiments/flags.h>
#include <alice/library/experiments/utils.h>

#include <search/begemot/rules/alice/response/proto/alice_response.pb.h>

namespace NAlice::NMegamind {

namespace {

TStatus FillCombinatorBaseRequest(NScenarios::TCombinatorRequest& combinatorRequest, TRTLogger& logger,
                                  const TSpeechKitRequest& skrView, const TRequest& requestModel) {
    auto& baseRequest = *combinatorRequest.MutableBaseRequest();

    baseRequest.SetRequestId(skrView->GetHeader().GetRequestId());
    *baseRequest.MutableClientInfo() = skrView->GetApplication();

    if (const auto& deviceId = skrView->GetRequest().GetDeviceState().GetDeviceId(); !deviceId.empty()) {
        *baseRequest.MutableClientInfo()->MutableDeviceId() = deviceId;
    }

    if (const auto& loc = requestModel.GetLocation(); loc.Defined()) {
        auto& location = *baseRequest.MutableLocation();
        location.SetLat(loc->GetLatitude());
        location.SetLon(loc->GetLongitude());
        location.SetAccuracy(loc->GetAccuracy());
        location.SetRecency(loc->GetRecency());
        location.SetSpeed(loc->GetSpeed());
    }

    baseRequest.MutableInterfaces()->CopyFrom(requestModel.GetInterfaces());
    TExpFlagsToStructVisitor{*baseRequest.MutableExperiments()}.Visit(skrView->GetRequest().GetExperiments());
    baseRequest.MutableOptions()->CopyFrom(requestModel.GetOptions());
    baseRequest.MutableUserPreferences()->CopyFrom(requestModel.GetUserPreferences());
    baseRequest.MutableUserClassification()->CopyFrom(requestModel.GetUserClassification());
    baseRequest.SetUserLanguage(static_cast<ELang>(IContext::ForceKnownLanguage(
        logger, LanguageByName(skrView->GetApplication().GetLang()), skrView.ExpFlags())));
    baseRequest.SetRandomSeed(skrView.GetSeed());
    baseRequest.SetServerTimeMs(requestModel.GetServerTimeMs());

    return Success();
}

void FillDataSources(NScenarios::TCombinatorRequest& /*combinatorRequest*/, IAppHostCtx& /*ahCtx*/,
                     const TCombinatorConfig& /*config*/)
{
    //TODO implement data sources
}

bool FillInput(NScenarios::TCombinatorRequest& combinatorRequest, const TCombinatorConfig& config, const TRequest& requestModel) {
    auto& input = *combinatorRequest.MutableInput();
    for (const auto& frame : requestModel.GetSemanticFrames()) {
        if (config.GetAcceptsAllFrames() || FindPtr(config.GetAcceptedFrames(), frame.GetName())) {
            *input.MutableSemanticFrames()->Add() = frame;
        }
    }
    // TODO fill input from event https://st.yandex-team.ru/MEGAMIND-2593
    return !input.GetSemanticFrames().empty() || config.GetAcceptsAllFrames();
}

TString ConstructCombinatorItemName(const TString& combinatorName) {
    return TString{AH_ITEM_COMBINATOR_REQUEST_PREFIX} + combinatorName;
}

} // namespace

TStatus TCombinatorSetupNodeHandler::Execute(IAppHostCtx& ahCtx) const {
    NScenarios::TCombinatorRequest combinatorBaseRequest;

    TFromAppHostSpeechKitRequest::TPtr skr;
    if (auto err = TFromAppHostSpeechKitRequest::Create(ahCtx).MoveTo(skr)) {
        return err;
    }
    TSpeechKitRequest skrView = *skr;

    TItemProxyAdapter& itemProxyAdapter = ahCtx.ItemProxyAdapter();

    const auto mementoData = GetMementoFromSpeechKitRequest(skrView, ahCtx.Log());
    mementoData.PutDataIntoContext(itemProxyAdapter);

    const auto errorOrRequest = itemProxyAdapter.GetFromContext<TRequestData>("mm_request_data");

    if (errorOrRequest.Error()) {
        return TError{TError::EType::Critical} << "Cannot get mm_request_data: " << *errorOrRequest.Error();
    }
    const auto requestModel = CreateRequest(errorOrRequest.Value());

    if (const auto error = FillCombinatorBaseRequest(combinatorBaseRequest, ahCtx.Log(), skrView, requestModel)) {
        return error;
    }

    NMegamindAppHost::TLaunchedCombinators launchedCombinators;
    for (const auto& [_, config] : ahCtx.GlobalCtx().CombinatorConfigRegistry().GetCombinatorConfigs()) {
        if (!config.GetEnabled() && !skrView.HasExpFlag(EXP_PREFIX_MM_ENABLE_COMBINATOR + config.GetName())) {
            continue;
        }
        NScenarios::TCombinatorRequest combinatorRequest(combinatorBaseRequest);
        if (!FillInput(combinatorRequest, config, requestModel)) {
            LOG_INFO(ahCtx.Log()) << "Combinator " << config.GetName() << " does not accepts any input";
            continue;
        }
        FillDataSources(combinatorRequest, ahCtx, config);
        itemProxyAdapter.PutIntoContext(combinatorRequest, ConstructCombinatorItemName(config.GetName()));
        launchedCombinators.AddCombinators()->SetName(config.GetName());
    }
    itemProxyAdapter.PutIntoContext(launchedCombinators, AH_ITEM_LAUNCHED_COMBINATORS);

    return Success();
}

void RegisterCombinatorHandlers(IGlobalCtx& globalCtx, TRegistry& registry) {
    static const TCombinatorSetupNodeHandler combinatorSetupHandler{globalCtx, /* useAppHostStreaming= */ false};
    registry.Add("/mm_combinators_setup", [](NAppHost::IServiceContext& ctx) { combinatorSetupHandler.RunSync(ctx); });
}

} // namespace NAlice::NMegaamind
