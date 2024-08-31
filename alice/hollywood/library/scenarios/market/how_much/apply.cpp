#include "apply.h"

#include "renderer.h"
#include <alice/hollywood/library/scenarios/market/how_much/proto/apply_arguments.pb.h>
#include <alice/hollywood/library/scenarios/market/common/report/proxy.h>

#include <util/generic/overloaded.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NMarket::NHowMuch {

namespace {

constexpr size_t MAX_GALLERY_COUNT = 5;

ui64 GetAvgPrice(const TVector<const TReportDocument*>& docs)
{
    // previous implementaion - https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/market/market_how_much_impl.cpp?rev=6922339#L313-368
    TVector<double> avgPrices;

    for (const auto* doc : docs) {
        std::visit(TOverloaded {
            [&avgPrices](const TReportOffer& offer) {
                const auto& prices = offer.GetPrices();
                avgPrices.push_back(
                    prices.Value
                    ? prices.Value.GetRef()
                    : prices.Min.GetOrElse(0) // других вариантов все равно нет
                );
            },
            [&avgPrices](const TReportModel& model) {
                const auto& prices = model.GetPrices();
                avgPrices.push_back(
                    prices.Avg // баг с бильярдными столами - у оффре только цена "от", нет средней
                    ? prices.Avg.GetRef()
                    : prices.Min
                );
            },
        }, *doc);
    }

    // TODO(bas1330) consider using std::nth_element
    Sort(avgPrices.begin(), avgPrices.end());
    auto middle = avgPrices.size() / 2;
    ui64 avgPrice = avgPrices.size() % 2 == 1
        ? avgPrices[middle]
        : (avgPrices[middle - 1] + avgPrices[middle]) / 2;
    return avgPrice ? avgPrice : 1;
}

}

void TApplyPrepareImpl::Do()
{
    const auto applyArgs = Ctx.RequestWrapper().UnpackArguments<TApplyArguments>();
    TSearchInfo searchInfo(applyArgs.GetRequestText(), applyArgs.GetRegionId());
    searchInfo.GoodState = EMarketGoodState::NEW;
    AddSearchInfoRequest(
        searchInfo,
        Scenario.ReportClient,
        Ctx,
        true /* allowRedirects */);
}

void TApplyRenderImpl::Do()
{
    const auto& searchInfo = GetSearchInfoOrThrow(Ctx);
    const auto& response = RetireReportPrimeResponse(Ctx);
    const auto applyArgs = Ctx.RequestWrapper().UnpackArguments<TApplyArguments>();
    const NGeobase::TId regionId = applyArgs.GetRegionId();
    const TStringBuf utterance = Ctx.RequestWrapper().Input().Utterance();

    TApplyResponseBuilder builder(&Ctx.NlgWrapper());
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    Scenario.SetAnalyticsInfo(bodyBuilder.CreateAnalyticsInfoBuilder());
    THowMuchRenderer renderer = MakeHowMuchRenderer(Ctx, bodyBuilder);

    const auto docs = GetSupportedDocs(response.GetDocuments(), searchInfo);
    if (docs.empty()) {
        if (Ctx.CanOpenUri()) {
            renderer.RenderYaSearch(utterance, Ctx.CreateUserLocation(regionId));
        } else {
            renderer.RenderYaSearchNotSupported();
        }
    } else {
        const auto avgPrice = GetAvgPrice(docs);
        const TStringBuf currency = std::visit([](const auto& doc) {
            return doc.GetPrices().Currency;
        }, *docs[0]);

        if (Ctx.SupportsDivCards()) {
            renderer.RenderAvgPrice(
                utterance,
                avgPrice,
                currency,
                CreateGalleryItems(docs, searchInfo),
                searchInfo.CreateMarketUrl(Scenario.UrlBuilder) /* = totalUrl */,
                response.GetTotal() /* = totalCount */
            );
        } else {
            renderer.RenderAvgPriceScreenless(avgPrice, currency);
        }
    }
    Ctx.AddResponse(std::move(builder));
}

// move to utils if it'll be needed somewhere else
TVector<const TReportDocument*> TApplyRenderImpl::GetSupportedDocs(
    const TVector<TReportDocument>& allDocs,
    const TSearchInfo& searchInfo) const
{
    TVector<const TReportDocument*> docs;
    if (searchInfo.Category) {
        if (Ctx.FastData().IsSupportedCategory(*searchInfo.Category)) {
            docs.reserve(allDocs.size());
            for (const auto& doc : allDocs) {
                docs.push_back(&doc);
            }
        }
    } else {
        auto isSupported = [this](const auto& doc) {
            for (const auto& category : doc.GetCategories()) {
                if (!Ctx.FastData().IsSupportedCategory(category)) {
                    return false;
                }
            }
            return true;
        };
        for (const auto& doc : allDocs) {
            if (std::visit(isSupported, doc)) {
                docs.push_back(&doc);
            }
        }
    }
    return docs;
}

TVector<TGalleryItem> TApplyRenderImpl::CreateGalleryItems(
    const TVector<const TReportDocument*>& docs,
    const TSearchInfo& searchInfo) const
{
    TVector<TGalleryItem> galleryItems;
    for (const auto& doc : docs) {
        TGalleryItem& item = galleryItems.emplace_back();
        std::visit([&item](const auto& doc) {
            item.Title = doc.GetTitle();
            item.Picture = doc.GetPicture();
            item.Currency = doc.GetPrices().Currency;
        }, *doc);
        std::visit(TOverloaded {
            [&item, &searchInfo, this](const TReportOffer& offer) {
                if (offer.GetPrices().Value) {
                    item.Price = offer.GetPrices().Value.GetRef();
                } else {
                    item.Price = offer.GetPrices().Min.GetOrElse(0);
                    item.IsFromPrice = true;
                }
                item.ActionUrl = Scenario.UrlBuilder.GetMarketOfferUrl(
                    offer.GetWareId(),
                    offer.GetCpc(),
                    searchInfo.RegionId
                );
            },
            [&item, &searchInfo, this](const TReportModel& model) {
                if (model.GetPrices().Default) {
                    item.Price = model.GetPrices().Default.GetRef();
                } else {
                    item.Price = model.GetPrices().Min;
                    item.IsFromPrice = true;
                }
                if (model.GetPrices().DefaultBeforeDiscount) {
                    item.PriceBeforeDiscount = model.GetPrices().DefaultBeforeDiscount.GetRef();
                }
                item.ActionUrl = Scenario.UrlBuilder.GetMarketModelUrl(
                    model.GetId(),
                    model.GetSlug(),
                    searchInfo.RegionId,
                    model.GetGlFilters()
                );
            },
        }, *doc);

        if (galleryItems.size() >= MAX_GALLERY_COUNT) {
            break;
        }
    }
    return galleryItems;
}

} // namespace NAlice::NHollywood::NMarket::NHowMuch
