#pragma once

#include <alice/hollywood/library/scenarios/order/proto/order.pb.h>

#include <alice/hollywood/library/framework/framework.h>

namespace NAlice::NHollywoodFw::NOrder {

inline constexpr TStringBuf SCENE_NAME_RELEVANT_ERROR = "order_relevant_order";

class TOrderRelevantErrorScene : public TScene<TOrderRelevantErrorSceneArgs> {

public:
    TOrderRelevantErrorScene(const TScenario* ownner);

    TRetMain Main(const TOrderRelevantErrorSceneArgs&,
                  const TRunRequest&,
                  TStorage&,
                  const TSource&) const override;
                
    static TRetResponse Render(const TOrderRelevantErrorRenderArgs&, TRender&);

};


}