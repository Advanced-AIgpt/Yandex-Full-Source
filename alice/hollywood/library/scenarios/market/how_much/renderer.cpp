#include "renderer.h"

namespace NAlice::NHollywood::NMarket::NHowMuch {

THowMuchRenderer::THowMuchRenderer(
        TResponseBodyBuilder& bodyBuilder,
        const TNlgData& baseNlgData,
        const TCommonResponseBuilder& commonBuilder)
    : BodyBuilder(bodyBuilder)
    , BaseNlgData(baseNlgData)
    , CommonBuilder(commonBuilder)
{}

void THowMuchRenderer::RenderAskRequestSlot()
{
    BodyBuilder.AddRenderedTextWithButtonsAndVoice(
        NlgTemplateName,
        TStringBuf("ask_request_slot"),
        {} /* = buttons */,
        BaseNlgData);
}

void THowMuchRenderer::RenderAvgPriceScreenless(ui64 avgPrice, TStringBuf currency)
{
    TNlgData textNlgData = BaseNlgData;
    textNlgData.Context["avg_price"] = avgPrice;
    textNlgData.Context["currency"] = currency;
    BodyBuilder.AddRenderedTextWithButtonsAndVoice(
        NlgTemplateName,
        "avg_price__no_cards",
        {} /* = buttons */,
        textNlgData);
    CommonBuilder.AddWhatCanYouDoSuggest();
}

void THowMuchRenderer::RenderAvgPrice(
    TStringBuf query,
    ui64 avgPrice,
    TStringBuf currency,
    const TVector<TGalleryItem>& items,
    TStringBuf totalUrl,
    ui64 totalCount)
{
    TNlgData textNlgData = BaseNlgData;
    textNlgData.Context["avg_price"] = avgPrice;
    textNlgData.Context["currency"] = currency;
    BodyBuilder.AddRenderedTextWithButtonsAndVoice(
        NlgTemplateName,
        "avg_price",
        {} /* = buttons */,
        textNlgData);
    CommonBuilder.AddGallery(items, totalUrl, totalCount);
    CommonBuilder.AddYaSearchSuggest(query);
    CommonBuilder.AddWhatCanYouDoSuggest();
}

void THowMuchRenderer::RenderYaSearch(TStringBuf query, const TUserLocation& userLocation)
{
    CommonBuilder.AddYaSearchCard(query, userLocation);
    CommonBuilder.AddYaSearchSuggest(query);
    CommonBuilder.AddWhatCanYouDoSuggest();
}

void THowMuchRenderer::RenderYaSearchNotSupported()
{
    CommonBuilder.AddYaSearchNotSupported();
    CommonBuilder.AddWhatCanYouDoSuggest();
}

THowMuchRenderer MakeHowMuchRenderer(TMarketBaseContext& ctx, TResponseBodyBuilder& bodyBuilder)
{
    return {
        bodyBuilder,
        ctx.BaseNlgData(),
        MakeCommonResponseBuilder(ctx, bodyBuilder)
    };
}

} // namespace NAlice::NHollywood::NMarket::NHowMuch
