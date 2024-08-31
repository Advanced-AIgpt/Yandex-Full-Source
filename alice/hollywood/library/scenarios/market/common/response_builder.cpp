#include "response_builder.h"

#include <alice/library/url_builder/url_builder.h>

namespace NAlice::NHollywood::NMarket {

NJson::TJsonValue TGalleryItem::ToJson() const
{
    NJson::TJsonValue jsoned;
    jsoned["title"] = Title;
    jsoned["picture"] = Picture.ToJson();
    jsoned["action_url"] = ActionUrl;

    jsoned["price"]["value"] = Price;
    jsoned["price"]["is_from_price"] = IsFromPrice;
    jsoned["price"]["currency"] = Currency;
    if (PriceBeforeDiscount) {
        jsoned["price"]["before_discount"] = PriceBeforeDiscount.GetRef();
    }

    return jsoned;
}

TCommonResponseBuilder::TCommonResponseBuilder(
        TResponseBodyBuilder& bodyBuilder,
        TNlgWrapper& nlgWrapper,
        const TNlgData& baseNlgData,
        const TClientInfo& clientInfo,
        EContentSettings contentRestrictionLevel,
        const TInterfaces& interfaces)
    : BodyBuilder(bodyBuilder)
    , NlgWrapper(nlgWrapper)
    , BaseNlgData(baseNlgData)
    , ClientInfo(clientInfo)
    , ContentRestrictionLevel(contentRestrictionLevel)
    , Interfaces(interfaces)
{}

void TCommonResponseBuilder::AddGallery(
    const TVector<TGalleryItem>& items,
    TStringBuf totalUrl,
    ui64 totalCount)
{
    NJson::TJsonArray jsonedItems;
    for (const auto& item : items) {
        jsonedItems.AppendValue(item.ToJson());
    }

    TNlgData nlgData = BaseNlgData;
    nlgData.Context["items"] = jsonedItems;
    nlgData.Context["total"]["url"] = totalUrl;
    nlgData.Context["total"]["count"] = totalCount;

    BodyBuilder.AddRenderedDivCard(NlgTemplateName, TStringBuf("gallery"), nlgData);
}

void TCommonResponseBuilder::AddYaSearchCard(
    TStringBuf query,
    const TUserLocation& userLocation)
{
    const TString uri = ::NAlice::GenerateSearchUri(
        ClientInfo,
        userLocation,
        ContentRestrictionLevel,
        query,
        Interfaces.GetCanOpenLinkSearchViewport()
    );
    NScenarios::TDirective openYaDirective;
    openYaDirective.MutableOpenUriDirective()->SetName(TString{"yandex_search"});
    openYaDirective.MutableOpenUriDirective()->SetUri(uri);

    const TString buttonCaption = NlgWrapper.RenderPhrase(
        NlgTemplateName,
        TStringBuf("yandex_search_button_caption"),
        BaseNlgData
    ).Text;
    // TODO(bas1330) it's better to create text card buttons explicitly but for now it's not supported enough
    TResponseBodyBuilder::TSuggest openYaSuggest {
        .Directives = { openYaDirective },
        .AutoDirective = openYaDirective,
        .ButtonForText = buttonCaption,
    };
    BodyBuilder.AddRenderedSuggest(std::move(openYaSuggest));

    BodyBuilder.AddRenderedTextWithButtonsAndVoice(
        NlgTemplateName,
        TStringBuf("yandex_search"),
        {} /* = buttons */,
        BaseNlgData
    );
}

void TCommonResponseBuilder::AddYaSearchNotSupported()
{
    BodyBuilder.AddRenderedTextWithButtonsAndVoice(
        NlgTemplateName,
        TStringBuf("yandex_search_not_supported"),
        {} /* = buttons */,
        BaseNlgData);
}

void TCommonResponseBuilder::AddYaSearchSuggest(TStringBuf query)
{
    TString queryStr = ToString(query);
    BodyBuilder.AddSearchSuggest().Title(queryStr).Query(std::move(queryStr));
}

void TCommonResponseBuilder::AddWhatCanYouDoSuggest()
{
    const TString text = NlgWrapper.RenderPhrase(
        NlgTemplateName,
        TStringBuf("what_can_you_do"),
        BaseNlgData
    ).Text;
    BodyBuilder.AddTypeTextSuggest(text);
}

TCommonResponseBuilder MakeCommonResponseBuilder(
    TMarketBaseContext& ctx,
    TResponseBodyBuilder& bodyBuilder)
{
    return {
        bodyBuilder,
        ctx.NlgWrapper(),
        ctx.BaseNlgData(),
        ctx.RequestWrapper().ClientInfo(),
        ctx.RequestWrapper().ContentRestrictionLevel(),
        ctx.RequestWrapper().Interfaces(),
    };
}

} // namespace NAlice::NHollywood::NMarket
