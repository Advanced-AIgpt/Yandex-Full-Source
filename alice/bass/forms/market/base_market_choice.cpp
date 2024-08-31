#include "base_market_choice.h"
#include "client/context_logger.h"
#include "dynamic_data.h"
#include "market_url_builder.h"
#include "settings.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <util/string/cast.h>
#include <util/string/join.h>

namespace NBASS {

namespace NMarket {

TBaseMarketChoice::TBaseMarketChoice(TMarketContext& ctx)
    : Ctx(ctx)
    , FilterWorker(Ctx)
{
}

TResultValue TBaseMarketChoice::Do()
{
    LOG(DEBUG) << "Form name " << Ctx.FormName() << Endl;
    if (Ctx.IsDebugMode()) {
        Ctx.RenderDebugInfo();
    }
    return DoImpl();
}

TString TBaseMarketChoice::GetSearchResultUrl(TMaybe<EMarketType> optionalMarketType) const
{
    return TMarketUrlBuilder(Ctx).GetMarketSearchUrl(
        optionalMarketType.GetOrElse(Ctx.GetChoiceMarketType()),
        Ctx.GetTextRedirect(),
        Ctx.UserRegion(),
        EMarketGoodState::NEW,
        false /* redirect */,
        Ctx.GetGalleryNumber(),
        Ctx.GetPrice()["from"].GetNumber(-1),
        Ctx.GetPrice()["to"].GetNumber(-1));
}

TString TBaseMarketChoice::GetCategoryResultUrl(
    TMaybe<EMarketType> optionalMarketType,
    const TMaybe<TCategory>& optionalCategory) const
{
    return TMarketUrlBuilder(Ctx).GetMarketCategoryUrl(
        optionalMarketType.GetOrElse(Ctx.GetChoiceMarketType()),
        optionalCategory.GetOrElse(Ctx.GetCategory()),
        Ctx.GetCgiGlFilters(),
        Ctx.UserRegion(),
        EMarketGoodState::NEW,
        Ctx.GetTextRedirect(),
        Ctx.GetRedirectCgiParams(),
        Ctx.GetGalleryNumber(),
        Ctx.GetPrice()["from"].GetNumber(-1),
        Ctx.GetPrice()["to"].GetNumber(-1),
        Ctx.GetSuggestTextRedirect());
}

TString TBaseMarketChoice::GetResultUrl(TMaybe<EMarketType> optionalMarketType)
{
    if (!Ctx.DoesCategoryExist()) {
        return GetSearchResultUrl(optionalMarketType);
    }
    return GetCategoryResultUrl(optionalMarketType);
}

bool TBaseMarketChoice::FillResult(const TVector<TReportResponse::TResult>& results)
{
    auto& documents = Ctx.CreateResultModels();

    size_t flagCount = Ctx.GetExperiments().MaxOffersCount();

    size_t maxCount = flagCount ? flagCount : MAX_MARKET_RESULTS;

    for (size_t i = 0; i < results.size() && documents.ArraySize() < maxCount; i++) {
        const auto& result = results[i];
        if (HasIllegalWarnings(result.GetWarnings())) {
            continue;
        }
        auto doc = GetResultDoc(result, documents.ArraySize() + 1);
        if (!doc.IsNull()) {
            documents.Push(doc);
        }
    }

    if (documents.ArrayEmpty()) {
        return false;
    }
    Ctx.SetGalleryNumber(Ctx.GetGalleryNumber() + 1);
    TString result = GetResultUrl();
    Ctx.SetResult(result);
    return true;
}

void TBaseMarketChoice::FillCtxFromParametricRedirect(const TReportResponse::TParametricRedirect& redirect)
{
    const auto& category = redirect.GetCategory();
    Ctx.SetCategory(category);
    for (const auto& kv : redirect.GetGlFilters()) {
        Ctx.AddGlFilter(kv.first, kv.second);
    }
    Ctx.SetFesh(redirect.GetFesh());

    Ctx.SetSuggestTextRedirect(redirect.GetSuggestText());
    Ctx.SetRedirectCgiParams(redirect.GetCgiParams());
    LOG(DEBUG) << "Parametric redirect. hid: " << category.GetHid() << " nid: " << category.GetNid() << Endl;
}

void TBaseMarketChoice::AppendTextRedirect(const TStringBuf text)
{
    const TStringBuf textRedirect = Ctx.GetTextRedirect();
    Y_ASSERT(IsLower(TUtf16String::FromUtf8(textRedirect)));
    TVector<TStringBuf> partList;
    Split(textRedirect, " ", partList);
    THashSet<TStringBuf> parts(partList.begin(), partList.end());

    TString lowerText = ToLowerUTF8(text);
    TVector<TStringBuf> newPartList;
    Split(lowerText, " ", newPartList);
    TStringBuilder newTextRedirect;
    newTextRedirect << textRedirect;

    for (auto newPart : newPartList) {
        if (!parts.contains(newPart)) {
            if (newTextRedirect) {
                newTextRedirect << " ";
            }
            newTextRedirect << newPart;
        }
    }
    Ctx.SetTextRedirect(newTextRedirect);
}

TReportRequest TBaseMarketChoice::MakeFilterRequestAsync(
    bool allowRedirects,
    TMaybe<EMarketType> marketType,
    const TMaybe<TCategory>& category,
    const TMaybe<TVector<i64>>& fesh)
{
    return TMarketClient(Ctx).MakeFilterRequestAsync(
        ToString(Ctx.GetTextRedirect()),
        ToString(Ctx.GetSuggestTextRedirect()),
        category.GetOrElse(Ctx.GetCategory()),
        fesh.GetOrElse(Ctx.GetFesh()),
        Ctx.GetPrice(),
        Ctx.GetCgiGlFilters(),
        Ctx.GetRedirectCgiParams(),
        allowRedirects,
        marketType);
}

TReportResponse TBaseMarketChoice::MakeFilterRequestWithRegionHandling(TMaybe<EMarketType> marketType)
{
    const auto& response = MakeFilterRequestAsync(true /* allowRedirects */, marketType).Wait();
    switch (response.GetRedirectType()) {
        case TReportResponse::ERedirectType::NONE:
            return response;
        case TReportResponse::ERedirectType::REGION:
            Ctx.SetUserRegion(response.GetRegionRedirect().GetUserRegion());
        case TReportResponse::ERedirectType::PARAMETRIC:
        case TReportResponse::ERedirectType::MODEL:
        case TReportResponse::ERedirectType::UNKNOWN:
            return MakeFilterRequestAsync(false /* allowRedirects */, marketType).Wait();
    }
}

bool TBaseMarketChoice::HasIllegalWarnings(const TVector<TWarning>& warnings)
{
    for (const auto& warning : warnings) {
        if (warning.IsIllegal()) {
            return true;
        }
    }
    return false;
}

NSc::TValue TBaseMarketChoice::GetResultDoc(
    const TReportResponse::TResult& result,
    ui32 galleryPosition)
{
    bool isVoicePurchase = result.HasBlueOffer();
    switch (result.GetType()) {
        case TReportResponse::TResult::EType::MODEL: {
            switch (Ctx.GetChoiceMarketType()) {
                case EMarketType::GREEN:
                    return Ctx.CreateJsonedModel(
                        result.GetModel(),
                        galleryPosition,
                        isVoicePurchase
                    );
                case EMarketType::BLUE:
                    return Ctx.CreateJsonedBlueOffer(
                        result.GetModel(),
                        galleryPosition,
                        isVoicePurchase
                    );
            }
        }
        case TReportResponse::TResult::EType::OFFER: {
            return Ctx.CreateJsonedOffer(
                result.GetOffer(),
                galleryPosition,
                isVoicePurchase
            );
        }
        default: {
            Y_ASSERT(false);
            return NSc::TValue();
        }
    }
}

TReportRequest TBaseMarketChoice::MakeSearchRequestAsync(
    TStringBuf query,
    TMaybe<EMarketType> marketType,
    bool allowRedirects,
    const TRedirectCgiParams& redirectParams)
{
    return TMarketClient(Ctx).MakeSearchRequestAsync(query, Ctx.GetPrice(), marketType, allowRedirects, Ctx.GetFesh(), redirectParams);
}

TReportResponse TBaseMarketChoice::MakeSearchRequest(
    TStringBuf query,
    TMaybe<EMarketType> marketType,
    bool allowRedirects,
    const TRedirectCgiParams& redirectParams)
{
    return MakeSearchRequestAsync(query, marketType, allowRedirects, redirectParams).Wait();
}

} // namespace NMarket

} // namespace NBASS
