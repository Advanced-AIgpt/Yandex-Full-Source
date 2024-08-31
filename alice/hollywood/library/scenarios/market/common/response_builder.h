#pragma once

#include "context.h"
#include "types.h"
#include "types/picture.h"

#include <alice/hollywood/library/response/response_builder.h>
#include <alice/library/geo/user_location.h>

namespace NAlice::NHollywood::NMarket {

/*
    Содержит общие методы формирования ответов маркетных сценариев.

    !!! Необходимо в nlg папке сценария добавить common_ru.nlg и записать:
    {% ext_nlgimport "alice/hollywood/library/scenarios/market/common/nlg/common_ru.nlg" %}
*/

struct TGalleryItem {
    TString Title;
    TPrice Price = 0;
    TMaybe<TPrice> PriceBeforeDiscount = Nothing();
    bool IsFromPrice = false;
    TString Currency { "RUR" };
    TString ActionUrl;
    TPicture Picture;

    NJson::TJsonValue ToJson() const;
};

class TCommonResponseBuilder {
public:
    TCommonResponseBuilder(
        TResponseBodyBuilder& bodyBuilder,
        TNlgWrapper& nlgWrapper,
        const TNlgData& baseNlgData,
        const TClientInfo& clientInfo,
        EContentSettings contentRestrictionLevel,
        const TInterfaces& interfaces);

    void AddGallery(
        const TVector<TGalleryItem>& items,
        TStringBuf totalUrl,
        ui64 totalCount);
    void AddYaSearchCard(
        TStringBuf query,
        const TUserLocation& userLocation);
    void AddYaSearchNotSupported();
    void AddYaSearchSuggest(TStringBuf query);
    void AddWhatCanYouDoSuggest();

private:
    TResponseBodyBuilder& BodyBuilder;
    TNlgWrapper& NlgWrapper;
    const TNlgData BaseNlgData;
    const TClientInfo& ClientInfo;
    EContentSettings ContentRestrictionLevel;
    const TInterfaces& Interfaces;

    static constexpr TStringBuf NlgTemplateName = "common";
};

TCommonResponseBuilder MakeCommonResponseBuilder(
    TMarketBaseContext& ctx,
    TResponseBodyBuilder& bodyBuilder);

} // namespace NAlice::NHollywood::NMarket
