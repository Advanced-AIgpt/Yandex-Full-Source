#include "order_relevant_error_scene.h"
#include "order.h"

#include <alice/hollywood/library/scenarios/order/proto/order.pb.h>

namespace NAlice::NHollywoodFw::NOrder {

TOrderRelevantErrorScene::TOrderRelevantErrorScene(const TScenario* owner)
    : TScene(owner, SCENE_NAME_RELEVANT_ERROR)
{
    RegisterRenderer(&TOrderRelevantErrorScene::Render);
}

TRetMain TOrderRelevantErrorScene::Main(
        const TOrderRelevantErrorSceneArgs& args,
        const TRunRequest& ,
        TStorage& ,
        const TSource& ) const
{

    TOrderRelevantErrorRenderArgs renderArgs;

    renderArgs.SetErrorType(args.GetErrorType());

    return TReturnValueRender(&TOrderRelevantErrorScene::Render, renderArgs);
}

TRetResponse TOrderRelevantErrorScene::Render(
        const TOrderRelevantErrorRenderArgs& args,
        TRender& render)
{
    switch (args.GetErrorType())
    {
        case EErrorType::ET_NOT_AUTHORIZED:
            render.CreateFromNlg("error", "render_unauthorized_user", args);
            break;
        case EErrorType::ET_IMPOSSIBLE_TO_DO_IT:
            render.CreateFromNlg("error", "render_impossible_to_do_it", args);
            break;
        case EErrorType::ET_UNKNOWN_ERROR_TYPE:
        case EErrorType_INT_MIN_SENTINEL_DO_NOT_USE_:
        case EErrorType_INT_MAX_SENTINEL_DO_NOT_USE_:
            render.CreateFromNlg("error", "render_error", args);
            break;
    }
    return TReturnValueSuccess();
}


}