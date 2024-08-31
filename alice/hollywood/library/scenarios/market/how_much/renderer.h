#pragma once

#include <alice/hollywood/library/scenarios/market/common/context.h>
#include <alice/hollywood/library/scenarios/market/common/response_builder.h>

#include <alice/hollywood/library/response/response_builder.h>
#include <alice/library/geo/user_location.h>

namespace NAlice::NHollywood::NMarket::NHowMuch {

/*
    Класс для формирования конечных ответов "сколько стоит"
*/
class THowMuchRenderer {
public:
    THowMuchRenderer(
        TResponseBodyBuilder& bodyBuilder,
        const TNlgData& baseNlgData,
        const TCommonResponseBuilder& commonBuilder);

    void RenderAskRequestSlot();
    void RenderAvgPriceScreenless(ui64 avgPrice, TStringBuf currency);
    void RenderAvgPrice(
        TStringBuf query,
        ui64 avgPrice,
        TStringBuf currency,
        const TVector<TGalleryItem>& items,
        TStringBuf totalUrl,
        ui64 totalCount);
    // TODO(bas1330) maybe make one YaSearchMethod?
    void RenderYaSearch(TStringBuf query, const TUserLocation& userLocation);
    void RenderYaSearchNotSupported();

private:
    TResponseBodyBuilder& BodyBuilder;
    const TNlgData BaseNlgData;
    TCommonResponseBuilder CommonBuilder;
    static constexpr TStringBuf NlgTemplateName = "how_much";

    void RenderDefaultBlocks();
};

THowMuchRenderer MakeHowMuchRenderer(TMarketBaseContext& ctx, TResponseBodyBuilder& bodyBuilder);

} // namespace NAlice::NHollywood::NMarket::NHowMuch
