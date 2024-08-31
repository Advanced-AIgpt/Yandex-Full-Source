#pragma once

#include <alice/hollywood/library/scenarios/order/proto/order.pb.h>

#include <alice/hollywood/library/framework/framework.h>

namespace NAlice::NHollywoodFw::NOrder {

inline constexpr TStringBuf SCENE_NAME_NOTIFICATION = "order_status_notification";
inline constexpr TStringBuf FRAME_ORDER_NOTIFICATION = "alice.order.notification";

struct TOrderStatusNotificationFrame : public TFrame {
    TOrderStatusNotificationFrame(const TRequest::TInput& input)
        : TFrame(input, FRAME_ORDER_NOTIFICATION)
    {
    }
};

class TOrderStatusNotificationScene : public TScene<TOrderStatusNotificationSceneArgs> {
public:
    TOrderStatusNotificationScene(const TScenario* owner);

    TRetSetup MainSetup(const TOrderStatusNotificationSceneArgs&, 
                        const TRunRequest&, 
                        const TStorage&) const override;

    TRetMain Main(const TOrderStatusNotificationSceneArgs&,
                  const TRunRequest&,
                  TStorage&,
                  const TSource&) const override;

    static TRetResponse Render(const TNotificationRenderArgs&, TRender&);
};

}