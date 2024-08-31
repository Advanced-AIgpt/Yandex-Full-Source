#include "serialize.h"

namespace NBASS {

namespace NMarket{

NSc::TValue SerializeModelPrices(const TModel& model)
{
    NSc::TValue jsoned;
    jsoned["min"] = model.GetMinPrice();
    if (model.HasAvgPrice()) {
        jsoned["avg"] = model.GetAvgPrice();
    }
    if (model.HasDefaultPrice()) {
        jsoned["default"] = model.GetDefaultPrice();
    }
    if (model.HasDefaultPriceBeforeDiscount()) {
        jsoned["before_discount"] = model.GetDefaultPriceBeforeDiscount();
    }
    return jsoned;
}

NSc::TValue SerializeOfferPrices(const TOffer& offer)
{
    NSc::TValue jsoned;
    jsoned["value"] = offer.GetPrice();
    if (offer.GetMinPrice()) {
        jsoned["min"] = offer.GetMinPrice();
    }
    if (offer.GetPriceBeforeDiscount()) {
        jsoned["before_discount"] = offer.GetPriceBeforeDiscount();
    }
    return jsoned;
}

NSc::TValue SerializeBlueOfferPrices(const NBassApi::TReportDocumentConst<TBoolSchemeTraits>& offer)
{
    NSc::TValue jsoned;
    jsoned["value"] = offer.Prices().Value();
    if (!offer.Prices().Discount().OldMin()->empty()) {
        jsoned["before_discount"] = offer.Prices().Discount().OldMin();
    }
    return jsoned;
}

void SerializeWarnings(const TVector<TWarning>& warnings, TWarningsScheme outputWarnings)
{
    for (const auto& warning : warnings) {
        auto outputWarning = outputWarnings.Add();
        outputWarning.Type() = warning.GetType();
        outputWarning.Value() = warning.GetValue();
        Y_ASSERT(outputWarning.Validate());
    }
}

void SerializePicture(const TPicture& picture, NBassApi::TPicture<TBoolSchemeTraits> outputPicture)
{
    outputPicture.Url() = picture.GetUrl();
    outputPicture.Height() = picture.GetHeight();
    outputPicture.Width() = picture.GetWidth();
    outputPicture.ContainerHeight() = picture.GetContainerHeight();
    outputPicture.ContainerWidth() = picture.GetContainerWidth();
    outputPicture.OriginalRatio() = picture.GetOriginalRatio();
}

void SerializePicture(const TPicture& picture, NSc::TValue& outputData)
{
    NBassApi::TPicture<TBoolSchemeTraits> outputPicture(&outputData);
    outputPicture.Url() = picture.GetUrl();
    outputPicture.Height() = picture.GetHeight();
    outputPicture.Width() = picture.GetWidth();
    outputPicture.ContainerHeight() = picture.GetContainerHeight();
    outputPicture.ContainerWidth() = picture.GetContainerWidth();
    outputPicture.OriginalRatio() = picture.GetOriginalRatio();
}

} // namespace NMarket

} // namespace NBASS
