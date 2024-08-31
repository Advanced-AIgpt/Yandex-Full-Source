#include "order_scene.h"

#include "order.h"
#include "order_algorithm.h"
#include "response_convertor.h"
#include "test_json_constants.h"

#include <alice/hollywood/library/scenarios/order/proto/order.pb.h>

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/request/experiments.h>

#include <alice/memento/proto/api.pb.h>
#include <alice/protos/data/scenario/order/config.pb.h>

#include <util/generic/algorithm.h>


namespace NAlice::NHollywoodFw::NOrder {

namespace {

constexpr TStringBuf REQUEST_KEY_ORDER = "order_request";
constexpr TStringBuf RESPONSE_KEY_ORDER = "order_response";

const TVector<NAlice::NOrder::TOrder::EStatus> COMPLETED_STATUS {
    NAlice::NOrder::TOrder::Succeeded, 
    NAlice::NOrder::TOrder::Canceled, 
    NAlice::NOrder::TOrder::Failed,
};
}

void SetDeliveryStatus(const NAlice::NOrder::TOrder& responseOrder, const TRunRequest& request, NAlice::NHollywoodFw::TOrderRenderArgs::TOrder* renderOrder) {
    if (const auto status = request.Flags().GetSubValue<TString>(NHollywood::EXP_HW_ORDER_STATUS_TEST)) {
        renderOrder->SetStatus(status.GetRef());
    } else {
        renderOrder->SetStatus(NAlice::NOrder::TOrder_EStatus_Name(responseOrder.GetStatus()));
    }
    if (!FindPtr(COMPLETED_STATUS.begin(), COMPLETED_STATUS.end(), responseOrder.GetStatus())) {
        renderOrder->SetCallDeliveryStatus(true);
        renderOrder->SetDeliveryEtaMin(responseOrder.GetEtaSeconds()/60);
        const NAlice::NOrder::TOrder::EDeliveryType deliveryType = responseOrder.GetDeliveryType().GetDeliveryType();
        if (deliveryType == NAlice::NOrder::TOrder::Curier || deliveryType == NAlice::NOrder::TOrder::Rover) {
            renderOrder->SetDeliveryType(NAlice::NOrder::TOrder_EDeliveryType_Name(deliveryType));
        } else {
            renderOrder->SetDeliveryType(NAlice::NOrder::TOrder_EDeliveryType_Name(NAlice::NOrder::TOrder::UnknownDeliveryType));
        }
    }
}

void FindShortestUniqueItem(const TSet<TString>& uniqueItems, NAlice::NHollywoodFw::TOrderRenderArgs::TOrder* renderOrder) {
    size_t minLength = 10000;
    TString title;
    for (const auto& item : uniqueItems) {
        if (!renderOrder->GetUniqueItemName() || minLength > item.size()) {
            minLength = item.size();
            title = item;
        }
    }
    renderOrder->SetUniqueItemName(title);
}

void PrepareOrders(const NAlice::NOrder::TProviderOrderResponse& response, const TRunRequest& request, TOrderRenderArgs& renderArgs) {
    TVector<TSet<TString>> uniqueItemsOfOrders(response.OrdersSize());
    for (size_t i = 0; i < response.OrdersSize(); i++) {
        TSet<TString> uniqueItems;
        for (const NAlice::NOrder::TOrder::TItem& item : response.GetOrders(i).GetItems()) {
            uniqueItems.insert(item.GetTitle());
        }
        uniqueItemsOfOrders[i] = uniqueItems;
    }
    const TVector<TSet<TString>> finalItemsOfOrders = FindUniqueElements(uniqueItemsOfOrders);
    for (size_t i = 0; i < finalItemsOfOrders.size(); i++) {
        const TSet<TString> uniqueItems = finalItemsOfOrders[i];
        const NAlice::NOrder::TOrder& responseOrder = response.GetOrders(i);
        NAlice::NHollywoodFw::TOrderRenderArgs::TOrder* renderOrder = renderArgs.AddOrders();
        renderOrder->SetOrdinalName(i + 1);
        SetDeliveryStatus(responseOrder, request, renderOrder);
        renderOrder->SetTotalQuantity(0);
        for (const NAlice::NOrder::TOrder::TItem& item : responseOrder.GetItems()) {
            renderOrder->SetTotalQuantity(renderOrder->GetTotalQuantity() + item.GetQuantity());
        }
        if (responseOrder.ItemsSize() == 1 && !uniqueItems.empty()) {
            renderOrder->SetCallUniqueItemName(true);
            renderOrder->SetOnlyOneItem(true);
            renderOrder->SetUniqueItemName(responseOrder.GetItems(0).GetTitle());
        } else if (uniqueItems.size() > 0) {
            renderOrder->SetCallUniqueItemName(true);
            FindShortestUniqueItem(uniqueItems, renderOrder);
        } 
    }
}

NAlice::NOrder::TProviderOrderResponse GetProtoResponse(const NJson::TJsonValue& jsonResponse, const TRunRequest& request) {

    if (const auto flag = request.Flags().GetSubValue<TString>(NHollywood::EXP_HW_ORDER_NUMBER_TEST)) {
        const NJson::TJsonValue customResponse = GetCustomJsonResponse(flag.GetRef());
        return ParseLavkaResponse(customResponse, request);
    }
    
    return ParseLavkaResponse(jsonResponse, request);
}

TOrderScene::TOrderScene(const TScenario* owner)
    : TScene(owner, SCENE_NAME_ORDER)
{
    RegisterRenderer(&TOrderScene::Render);
}

TRetMain TOrderScene::Main(
        const TOrderSceneArgs&,
        const TRunRequest& request,
        TStorage& storage,
        const TSource& source) const
{
    TOrderRenderArgs renderArgs;

    const NJson::TJsonValue jsonResponse = source.GetHttpResponseJson(RESPONSE_KEY_ORDER, false);

    if (jsonResponse.IsNull()) {
        renderArgs.SetIncorrectResponse(true);
        return TReturnValueRender(&TOrderScene::Render, renderArgs);
    }

    NAlice::NOrder::TProviderOrderResponse response = GetProtoResponse(jsonResponse, request);

    SortBy(response.MutableOrders()->begin(), response.MutableOrders()->end(), [](const NAlice::NOrder::TOrder& a) {
        return a.GetCreatedDate(); 
    });

    LOG_INFO(request.Debug().Logger()) << response.ShortDebugString();

    if (response.OrdersSize() == 0) {
        return TReturnValueRender(&TOrderScene::Render, renderArgs);
    }

    for (const NAlice::NOrder::TOrder& responseOrder : response.GetOrders()) {
        if (responseOrder.GetStatus() == NAlice::NOrder::TOrder::UnknownStatus) {
            renderArgs.SetIncorrectResponse(true);
            return TReturnValueRender(&TOrderScene::Render, renderArgs);
        }
    }

    if (const auto callItems = request.Flags().GetSubValue<bool>(NHollywood::EXP_HW_ORDER_CALLITEMS)) {
        renderArgs.SetCallItems(callItems.GetRef());
    } else {
        renderArgs.SetCallItems(!storage.GetMementoUserConfig().GetOrderStatusConfig().GetHideItemNames());
    }

    PrepareOrders(response, request, renderArgs);

    return TReturnValueRender(&TOrderScene::Render, renderArgs);
}

static THttpProxyRequest SetupOrderRequest(
        const TOrderSceneArgs& ,
        const TRunRequest& request,
        const NScenarios::TRequestMeta& requestMeta, 
        TRTLogger& logger) 
{

    NHollywood::THttpProxyRequestBuilder httpRequestBuilder( "/internal/v1/orders/v1/state" , requestMeta, logger);
    httpRequestBuilder.SetMethod(NAppHostHttp::THttpRequest_EMethod::THttpRequest_EMethod_Post);
    httpRequestBuilder.SetBody("{\"yandex_uid\":\""+ request.GetPUID() +"\"}");
    return std::move(httpRequestBuilder.Build());
}

TRetSetup TOrderScene::MainSetup(
        const TOrderSceneArgs& args, 
        const TRunRequest& request, 
        const TStorage&) const 
{

    TSetup setup(request);
    setup.Attach(SetupOrderRequest(args, request ,request.GetRequestMeta(), request.Debug().Logger()), REQUEST_KEY_ORDER);   
    return setup;
}

TRetResponse TOrderScene::Render(
        const TOrderRenderArgs& args,
        TRender& render)
{   
    if (args.GetIncorrectResponse()) {
        render.CreateFromNlg("order", "render_incorrect_response", args);
    } else if (args.OrdersSize() == 0) {
        render.CreateFromNlg("order", "render_no_orders", args);
    } else if (args.OrdersSize() == 1) {
        render.CreateFromNlg("order", "render_one_order", args);
    } else {
        render.CreateFromNlg("order", "render_multiple_orders", args);
    }

    return TReturnValueSuccess();
}

}  // namespace NAlice::NHollywood::NOrder
