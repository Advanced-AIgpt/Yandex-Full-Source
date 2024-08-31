#include "utils.h"

namespace NAlice::NHollywood::NResponseMerger {

TDiv2Card Div2CardFromRenderResponse(const NRenderer::TRenderResponse* renderResponse, TRTLogger& logger) {
    TDiv2Card div2Card;
    if (renderResponse->HasDiv2Body()) {
        div2Card.MutableBody()->CopyFrom(renderResponse->GetDiv2Body());
    } else if (renderResponse->HasStringDiv2Body()) {
        *div2Card.MutableStringBody() = std::move(renderResponse->GetStringDiv2Body());
    } else { 
        LOG_ERROR(logger) << "Div2Card has neither Div2Body nor StringDiv2Body";
    }
    *div2Card.MutableCardName() = renderResponse->GetCardName();
    TDiv2Id div2Id;
    div2Id.SetCardName(renderResponse->GetCardName());
    div2Id.SetCardId(renderResponse->GetCardId());
    *div2Card.MutableId() = std::move(div2Id);
    if (!renderResponse->GetGlobalDiv2Templates().empty()) {
        auto* templates = div2Card.MutableGlobalTemplates();
        for (auto const& [templateName, div2Template] : renderResponse->GetGlobalDiv2Templates()) {
            auto div2CardTemplate = ::NAlice::TDiv2Card_Template();
            if (div2Template.HasBody()) {
                div2CardTemplate.MutableBody()->CopyFrom(div2Template.GetBody());
            } else if (div2Template.HasStringBody()) {
                *div2CardTemplate.MutableStringBody() = std::move(div2Template.GetStringBody());
            } else {
                LOG_ERROR(logger) << "Div2CardTemplate has neither Body nor StringBody";
            }
            (*templates)[TString(templateName)] = div2CardTemplate;
        }
    }

    return std::move(div2Card);
}

} // namespace NAlice::NHollywood::NResponseMerger
