#include "order.h"

#include "order_relevant_error_scene.h"
#include "order_scene.h"
#include "order_status_notification_scene.h"

#include <alice/hollywood/library/scenarios/order/nlg/register.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/json/json.h>

namespace NAlice::NHollywoodFw::NOrder {

namespace {
constexpr TStringBuf INTENT_RELEVANT_ERROR = "alice.order.relevant_error";
}

HW_REGISTER(TOrderScenario);

TOrderScenario::TOrderScenario()
    : TScenario(NProductScenarios::ORDER)
{
    Register(&TOrderScenario::Dispatch);
    
    RegisterScene<TOrderScene>([this]() {
        RegisterSceneFn(&TOrderScene::Main);
        RegisterSceneFn(&TOrderScene::MainSetup);
    });
    RegisterScene<TOrderStatusNotificationScene>([this]() {
        RegisterSceneFn(&TOrderStatusNotificationScene::Main);
        RegisterSceneFn(&TOrderStatusNotificationScene::MainSetup);
    });
    RegisterScene<TOrderRelevantErrorScene>([this]() {
        RegisterSceneFn(&TOrderRelevantErrorScene::Main);
    });

    RegisterRenderer(&TOrderScenario::RenderIrrelevant);

    // Additional functions
    SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NOrder::NNlg::RegisterAll);
}

TRetScene TOrderScenario::Dispatch(
        const TRunRequest& runRequest,
        const TStorage& ,
        const TSource&) const
{

    if (runRequest.GetPUID().empty()) {
        TOrderRelevantErrorSceneArgs args;
        args.SetErrorType(EErrorType::ET_NOT_AUTHORIZED);
        return TReturnValueScene<TOrderRelevantErrorScene>(args, INTENT_RELEVANT_ERROR.data());
    }

    const TOrderFrame frameOrder(runRequest.Input());
    if (frameOrder.Defined()) { 
        if (frameOrder.ProviderName.Defined() && *frameOrder.ProviderName == "market"){
            return TReturnValueRenderIrrelevant(&TOrderScenario::RenderIrrelevant, {});
        }
        if (frameOrder.ProviderName.Defined() && *frameOrder.ProviderName != "lavka"){
            TOrderRelevantErrorSceneArgs args;
            args.SetErrorType(EErrorType::ET_IMPOSSIBLE_TO_DO_IT);
            return TReturnValueScene<TOrderRelevantErrorScene>(args, INTENT_RELEVANT_ERROR.data());
        }
        TOrderSceneArgs args;
        return TReturnValueScene<TOrderScene>(args, frameOrder.GetName());
    }

    const TOrderStatusNotificationFrame frameNotification(runRequest.Input());
    if (frameNotification.Defined()) {
        TOrderStatusNotificationSceneArgs args;
        return TReturnValueScene<TOrderStatusNotificationScene>(args, frameNotification.GetName());
    }

    LOG_ERR(runRequest.Debug().Logger()) << "Semantic frames not found";
    return TReturnValueRenderIrrelevant(&TOrderScenario::RenderIrrelevant, {});
}

TRetResponse TOrderScenario::RenderIrrelevant(
        const TOrderRenderIrrelevant&,
        TRender& render)
{
    render.CreateFromNlg("error", "render_error", NJson::TJsonValue{});
    return TReturnValueSuccess();
}

}  // namespace NAlice::NHollywood::NOrder
