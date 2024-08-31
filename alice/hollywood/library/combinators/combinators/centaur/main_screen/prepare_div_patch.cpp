#include "prepare_div_patch.h"

#include <alice/protos/api/renderer/api.pb.h>
#include <alice/protos/data/scenario/data.pb.h>
#include <alice/protos/div/div2id.pb.h>

namespace NAlice::NHollywood::NCombinators {

using namespace NAlice::NScenarios;

TPrepareDivPatch::TPrepareDivPatch(THwServiceContext& ctx, const TCombinatorRequest& combinatorRequest)
    : Ctx(ctx),
      Request(combinatorRequest, Ctx.Logger(), Ctx),
      CombinatorContextWrapper(Ctx, Request, ResponseForRenderer),
      WidgetResponses(PrepareWidgetResponses(CombinatorContextWrapper))
{
}

void TPrepareDivPatch::Do(const TSemanticFrame& collectMainScreenSemanticFrame) {
    LOG_INFO(Ctx.Logger()) << "CentaurMainScreen combinator in div patch stage";
    TFrame frame = TFrame::FromProto(collectMainScreenSemanticFrame);
    const auto widgetConfigDataSlot = frame.FindSlot("widget_config_data_slot");
    const auto widgetMainScreenPositionSlot = frame.FindSlot("widget_main_screen_position");

    Y_ENSURE(widgetConfigDataSlot.IsValid() && widgetMainScreenPositionSlot.IsValid(),
        "Invalid widget data for div patch in semantic frame " << CENTAUR_COLLECT_MAIN_SCREEN_FRAME);

    const auto& newWidgetConfigData = collectMainScreenSemanticFrame.GetTypedSemanticFrame().GetCentaurCollectMainScreenSemanticFrame()
                                .GetWidgetConfigDataSlot().GetWidgetConfigDataValue();
    const auto& mainScreenPosition = collectMainScreenSemanticFrame.GetTypedSemanticFrame().GetCentaurCollectMainScreenSemanticFrame()
                                .GetWidgetMainScreenPosition().GetWidgetPositionValue();
    const auto widgetType = newWidgetConfigData.GetWidgetType();

    auto& patchViewDirective = *ResponseForRenderer.MutableResponseBody()
                                ->MutableLayout()
                                ->AddDirectives()
                                ->MutablePatchViewDirective();
    patchViewDirective.SetName("Patch View");
    NAlice::TDiv2Id div2Id;
    div2Id.SetCardName(MAIN_SCREEN_DIV_CARD_NAME);
    div2Id.SetCardId(MY_SCREEN_DIV_CARD_ID);
    *patchViewDirective.MutableApplyTo() = div2Id;

    NData::TCentaurWidgetCardData card = MyScreenCard(newWidgetConfigData, mainScreenPosition.GetColumn(),
        mainScreenPosition.GetRow(), WidgetResponses, CombinatorContextWrapper);
    NData::TScenarioData scenarioData;
    auto& scenarioWidgetData = *scenarioData.MutableCentaurScenarioWidgetData();
    *scenarioWidgetData.MutableWidgetType() = widgetType;
    *scenarioWidgetData.MutableWidgetCards()->Add() = card;

    NRenderer::TDivRenderData divPatchData;
    divPatchData.SetCardName(div2Id.GetCardName());
    divPatchData.SetCardId(div2Id.GetCardId());
    *divPatchData.MutableDivPatchData() = scenarioData;
    
    Ctx.AddProtobufItemToApphostContext(divPatchData, RENDER_DATA_ITEM);
    Ctx.AddProtobufItemToApphostContext(ResponseForRenderer, RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood::NCombinators
