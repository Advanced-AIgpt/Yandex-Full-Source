#include "prepare_widget_gallery.h"

#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/scenarios/frame.pb.h>
#include <alice/memento/proto/device_configs.pb.h>
#include <alice/protos/api/renderer/api.pb.h>
#include <alice/protos/data/scenario/centaur/main_screen.pb.h>
#include <alice/protos/data/scenario/centaur/my_screen/widgets.pb.h>
#include <alice/protos/data/scenario/data.pb.h>

namespace NAlice::NHollywood::NCombinators {

using namespace NAlice::NScenarios;

TPrepareWidgetGallery::TPrepareWidgetGallery(THwServiceContext& ctx, const TCombinatorRequest& combinatorRequest)
    : Ctx(ctx),
      Request(combinatorRequest, Ctx.Logger(), Ctx),
      CombinatorContextWrapper(Ctx, Request, ResponseForRenderer),
      WidgetResponses(PrepareWidgetResponses(CombinatorContextWrapper))
{
}

void TPrepareWidgetGallery::Do(const TSemanticFrame& semanticFrame) {
    const auto widgetPosition = semanticFrame.GetTypedSemanticFrame().GetCentaurCollectMainScreenSemanticFrame()
                                .GetWidgetGalleryPosition().GetWidgetPositionValue();
    const auto column = widgetPosition.GetColumn();
    const auto row = widgetPosition.GetRow();

    NRenderer::TDivRenderData divRendererData;
    divRendererData.SetCardId(WIDGET_GALLERY_DIV_CARD_ID);
    auto& widgetGalleryData = *divRendererData.MutableScenarioData()->MutableCentaurWidgetGalleryData();
    widgetGalleryData.SetId(WIDGET_GALLERY_DIV_CARD_ID);

    auto& showViewDirective = *ResponseForRenderer.MutableResponseBody()
                            ->MutableLayout()
                            ->AddDirectives()
                            ->MutableShowViewDirective();
    showViewDirective.SetName("show_view");
    showViewDirective.SetCardId(WIDGET_GALLERY_DIV_CARD_ID);
    showViewDirective.MutableLayer()->MutableContent();
    showViewDirective.SetInactivityTimeout(TShowViewDirective_EInactivityTimeout_Long);
    showViewDirective.SetDoNotShowCloseButton(true);

    const auto columns = GetCentaurWidgetDeviceConfig(CombinatorContextWrapper).GetColumns();

    for (const auto& widgetTypeResponse : WidgetResponses) {
        TString widgetToAddType = widgetTypeResponse.first;
        for (const auto& widgetResponse : widgetTypeResponse.second) {
            bool widgetExists = false;
            for (const auto& column : columns) {
                for (const auto& widget : column.GetWidgetConfigs()) {
                    if (widget.GetWidgetType() == widgetToAddType && (widget.GetId().empty() || widget.GetId() == widgetResponse.second.GetId())) {
                        widgetExists = true;
                    }
                }
            }
            if (!widgetExists) {
                NData::TCentaurWidgetConfigData widgetConfigToAdd;
                widgetConfigToAdd.SetId(widgetResponse.second.GetId());
                widgetConfigToAdd.SetWidgetType(widgetToAddType);
                *widgetGalleryData.AddWidgetCards() = WidgetGalleryCard(widgetConfigToAdd, column, row);
            }
        }
    }

    {
        NData::TCentaurWidgetConfigData widgetConfig;
        widgetConfig.SetWidgetType("vacant");
        *widgetGalleryData.AddWidgetCards() = WidgetGalleryCard(widgetConfig, column, row);
    }

    Ctx.AddProtobufItemToApphostContext(divRendererData, RENDER_DATA_ITEM);
    Ctx.AddProtobufItemToApphostContext(ResponseForRenderer, RESPONSE_ITEM);
}

NData::TCentaurWidgetCardData TPrepareWidgetGallery::WidgetGalleryCard(const NData::TCentaurWidgetConfigData& widgetConfigData, const int column, const int row) {
    NData::TCentaurWidgetCardData card;
    TString widgetType = widgetConfigData.GetWidgetType();
    if (widgetType == "vacant") {
        card = MyScreenVacantCard(column, row, CombinatorContextWrapper);
    } else {
        card = MyScreenWidgetCard(widgetConfigData, WidgetResponses, Ctx.Logger());
    }
    
    if (Request.HasExpFlag(TYPED_ACTION_EXP_FLAG_NAME)) {
        *card.MutableTypedAction() = AddWidgetFromGalleryActionTyped(widgetConfigData, column, row);
    } else {
        card.SetAction(AddWidgetFromGalleryAction(widgetConfigData, column, row, CombinatorContextWrapper));
    }

    return card;
}

} // namespace NAlice::NHollywood::NCombinators
