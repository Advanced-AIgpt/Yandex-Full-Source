#include "order_status_notification_scene.h"
#include "order.h"

#include <alice/hollywood/library/scenarios/order/proto/order.pb.h>

#include <alice/hollywood/library/http_proxy/http_proxy.h>

namespace NAlice::NHollywoodFw::NOrder {

namespace {
    constexpr TStringBuf REQUEST_KEY_NOTIFICATION = "order_request";
    constexpr TStringBuf RESPONSE_KEY_NOTIFICATION = "order_response";
}

TOrderStatusNotificationScene::TOrderStatusNotificationScene(const TScenario* owner)
    : TScene(owner, SCENE_NAME_NOTIFICATION)
{
    RegisterRenderer(&TOrderStatusNotificationScene::Render);
}

TRetMain TOrderStatusNotificationScene::Main(
        const TOrderStatusNotificationSceneArgs& args,
        const TRunRequest& request,
        TStorage& storage,
        const TSource& source) const
{
    Y_UNUSED(args);
    Y_UNUSED(storage);
    Y_UNUSED(source);

    const NJson::TJsonValue jsonResponse = source.GetHttpResponseJson(RESPONSE_KEY_NOTIFICATION, false);
    LOG_INFO(request.Debug().Logger()) << "Here is notification scenario";
    LOG_INFO(request.Debug().Logger()) << source.Size();
    LOG_INFO(request.Debug().Logger()) << jsonResponse;
    LOG_INFO(request.Debug().Logger()) << request.GetPUID();

    TNotificationRenderArgs renderArgs;
    return TReturnValueRender(&TOrderStatusNotificationScene::Render, renderArgs);
}

static THttpProxyRequest SetupNotificationRequest(
        const TOrderStatusNotificationSceneArgs& ,
        const TRunRequest& request,
        const NScenarios::TRequestMeta& requestMeta, 
        TRTLogger& logger) 
{

    NHollywood::THttpProxyRequestBuilder httpRequestBuilder( "/internal/v1/orders/v1/state" , requestMeta, logger);
    httpRequestBuilder.SetMethod(NAppHostHttp::THttpRequest_EMethod::THttpRequest_EMethod_Post);
    httpRequestBuilder.SetBody("{\"yandex_uid\":\""+ request.GetPUID() +"\"}");
    //TODO - изменить запрос под сцену
    return std::move(httpRequestBuilder.Build());
}

TRetSetup TOrderStatusNotificationScene::MainSetup(
        const TOrderStatusNotificationSceneArgs& args, 
        const TRunRequest& request, 
        const TStorage&) const 
{

    TSetup setup(request);
    setup.Attach(SetupNotificationRequest(args, request ,request.GetRequestMeta(), request.Debug().Logger()), REQUEST_KEY_NOTIFICATION);   
    return setup;
}

TRetResponse TOrderStatusNotificationScene::Render(
        const TNotificationRenderArgs& args,
        TRender& render)
{
    render.CreateFromNlg("notification", "render_notification_scene", args);
    return TReturnValueSuccess();
}

}  // namespace NAlice::NHollywood::NOrder
