#include "market_how_much_impl.h"

#include "clear_request.h"
#include "dynamic_data.h"
#include "market_url_builder.h"

#include <alice/bass/forms/market/client/context_logger.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <util/generic/algorithm.h>
#include <util/generic/utility.h>

#include <util/string/join.h>
#include <util/string/split.h>

namespace NBASS {

namespace NMarket {

const size_t MAX_SEARCH_RESULT_GALLERY_COUNT = 5;

TMarketHowMuchRequestImpl::TMarketHowMuchRequestImpl(TMarketContext& ctx)
    : CurrentRequest()
    , Ctx(ctx)
    , FilterWorker(Ctx)
    , GeoSupport(Ctx.Ctx().GlobalCtx())
{
}

TResultValue TMarketHowMuchRequestImpl::Do() {
    LOG(DEBUG) << "Form name " << Ctx.FormName() << Endl;
    Ctx.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::HOW_MUCH);
    if (Ctx.IsDebugMode()) {
        Ctx.RenderDebugInfo();
    }

    if (!GeoSupport.IsMarketSupportedForGeoId(Ctx.UserRegion(), Ctx.GetScenarioType())) {
        return HandleEmptySerp();
    }

    CurrentRequest = Ctx.Request();
    if (TDynamicDataFacade::ContainsVulgarQuery(CurrentRequest)) {
        Ctx.SetVulgar();
        return TResultValue();
    }
    LOG(DEBUG) << "New request: " << CurrentRequest << Endl;
    // Заранее проставим маркетный поисковый урл,
    // чтоб в случае ошибок, неответа по таймауту или других проблем
    // форма показывала нормальную ссылку
    Ctx.SetResult(TMarketUrlBuilder(Ctx).GetMarketSearchUrl(
        DEFAULT_MARKET_TYPE, CurrentRequest, Ctx.UserRegion(), EMarketGoodState::NEW));

    if (TUtf32String::FromUtf8(CurrentRequest).size() <= 1) {
        LOG(DEBUG) << "Request \"" << CurrentRequest << "\" is too short" << Endl;
        return HandleEmptySerp();
    }

    auto response = TMarketClient(Ctx).MakeSearchRequest(CurrentRequest);
    return HandleResponseWithPossibleRedirect(response);
}

TResultValue TMarketHowMuchRequestImpl::HandleResponseWithPossibleRedirect(
    const TReportResponse& response,
    size_t callCount)
{
    if (callCount > 1) {
        /* Мы можем вызвать этот метод рекурсивно ТОЛЬКО 1 раз при региональном редиректе. */
        return TError(
            NBASS::TError::EType::MARKETERROR,
            "Cannot run report redirect handling twice"
        );
    }
    callCount++;

    if (response.HasError()) {
        return response.GetError();
    }

    switch (response.GetRedirectType()) {
        case TReportResponse::ERedirectType::MODEL: {
            auto modelId = response.GetModelRedirect().GetModelId();
            LOG(DEBUG) << "ModelId exists: " << modelId << Endl;

            return HandleModel(modelId);
        }
        case TReportResponse::ERedirectType::PARAMETRIC: {
            auto redirect = response.GetParametricRedirect();
            const auto& category = redirect.GetCategory();
            const auto& cgiParams = redirect.GetCgiParams();
            LOG(DEBUG) << "Parametric redirect."
                       << " hid: " << category.GetHid()
                       << " nid: " << category.GetNid()
                       << " rs: " << cgiParams.ReportState << Endl;

            return HandleParametric(category, cgiParams, redirect.GetGlFilters(), redirect.GetText(), redirect.GetSuggestText());
        }
        case TReportResponse::ERedirectType::NONE: {
            LOG(DEBUG) << "No redirect" << Endl;

            return HandleOther(response.GetResults());
        }
        case TReportResponse::ERedirectType::REGION: {
            LOG(DEBUG) << "Region redirect" << Endl;

            const auto redirect = response.GetRegionRedirect();
            Ctx.SetUserRegion(redirect.GetUserRegion());
            return HandleResponseWithPossibleRedirect(
                TMarketClient(Ctx).MakeSearchRequest(
                    CurrentRequest,
                    NSc::TValue(), /* price */
                    Nothing(),     /* marketChoiceType */
                    callCount <= 1 ? true : false /* allowRedirects */),
                callCount);
        }
        case TReportResponse::ERedirectType::UNKNOWN: {
            LOG(DEBUG) << "Unknown redirect" << Endl;

            const auto& noRedirResponse = TMarketClient(Ctx).MakeSearchRequest(
                CurrentRequest,
                NSc::TValue(), /* price */
                Nothing(),     /* marketChoiceType */
                false /* allowRedirects */);
            if (noRedirResponse.HasError()) {
                return noRedirResponse.GetError();
            }
            return HandleOther(noRedirResponse.GetResults());
        }
    }
}

TResultValue TMarketHowMuchRequestImpl::HandleModel(TModelId modelId)
{
    // запрос для получения данных по модели
    auto modelResponse = TMarketClient(Ctx).MakeSearchModelRequest(modelId);
    if (modelResponse.HasError()) {
        return modelResponse.GetError();
    }
    auto results = modelResponse.GetResults();
    if (results.empty()) {
        LOG(DEBUG) << "ModelId " << modelId << " does not exists" << Endl;
        return HandleEmptySerp();
    }
    auto result = results[0];
    const auto& model = result.GetModel();
    Ctx.SetModel(model);
    Ctx.SetCurrency(model.GetCurrency());

    auto offers = result.GetModelOffers();
    if (offers.empty()) {
        LOG(DEBUG) << "No offers for modelId: " << modelId << Endl;
        return HandleEmptySerp();
    }

    Ctx.AddModelSnippet(model);
    Ctx.RenderHowMuchModel();

    return TResultValue();
}

TResultValue TMarketHowMuchRequestImpl::HandleParametric(
    const TCategory& category,
    const TRedirectCgiParams& redirectParams,
    const TCgiGlFilters& filters,
    const TString& text,
    const TString& suggestText)
{
    // если маркетный редирект в неподдерживаемую категорию, то возвращаемся в поиск
    if (!TDynamicDataFacade::IsSupportedCategory(category.GetHid())) {
        LOG(DEBUG) << category.GetHid() << " contains in not supported categories" << Endl;
        return HandleEmptySerp();
    }

    Ctx.SetCategory(category);
    for (const auto& kv : filters) {
        Ctx.AddGlFilter(kv.first, kv.second);
    }

    // выставим урл ответа до похода за данными
    FillResultUrls(TMarketUrlBuilder(Ctx).GetMarketCategoryUrl(
        DEFAULT_MARKET_TYPE,
        category,
        filters,
        Ctx.UserRegion(),
        EMarketGoodState::NEW,
        text,
        redirectParams,
        0,
        -1,
        -1,
        suggestText)
    );

    // запрос за моделями категории с выбранными gl-фильтрами
    auto categoryResponse = TMarketClient(Ctx).MakeCategoryRequest(
                                      category,
                                      Ctx.GetCgiGlFilters(),
                                      redirectParams,
                                      text,
                                      suggestText);
    if (categoryResponse.HasError()) {
        return categoryResponse.GetError();
    }

    // определяем текстовки фильтров и их значений
    // Например: тип "фен", "подача холодного воздуха", производитель "Philips", "ионизация"
    const auto resolvedValues = FilterWorker.ResolveFiltersNameAndValue(categoryResponse.GetFilters());
    TVector<TStringBuf> descriptions;
    for (const auto& kv : filters) {
        const auto& found = resolvedValues.find(kv.first);
        if (found != resolvedValues.end()) {
            descriptions.push_back(found->second);
        }
    }
    if (!descriptions.empty()) {
        Ctx.SetGlFilterDescription(JoinSeq(", ", descriptions));
    }

    const auto& results = categoryResponse.GetResults();
    if (results.empty()) {
        return HandleEmptySerp();
    }

    Ctx.AddHowMuchTotal(categoryResponse.GetTotal());
    FillAndRenderPopularGoods(results);
    for (const auto& category : results[0].GetCategories()) {
        if (category["isLeaf"].GetBool()) {
            Ctx.SetPopularGoodCategoryName(category["fullName"].GetString());
            break;
        }
    }

    return TResultValue();
}

void TMarketHowMuchRequestImpl::FillResultUrls(const TString& url)
{
    Ctx.SetPopularGoodUrl(url);
    Ctx.SetResult(url);
    Ctx.SetCurrentResult(url);
}

TResultValue TMarketHowMuchRequestImpl::HandleOther(const TVector<TReportResponse::TResult>& allResults)
{
    TVector<TReportResponse::TResult> results{Reserve(allResults.size())};
    for (const auto& result : allResults) {
        if (TDynamicDataFacade::IsSupportedCategory(result.GetCategories())) {
            results.emplace_back(std::move(result));
        } else {
            LOG(DEBUG) << "contains in not supported category" << Endl;
        }
    }

    if (results.empty()) {
        return HandleEmptySerp();
    }
    Ctx.AddHowMuchTotal(allResults.size());
    FillResultUrls(TMarketUrlBuilder(Ctx).GetMarketSearchUrl(
        DEFAULT_MARKET_TYPE, CurrentRequest, Ctx.UserRegion(), EMarketGoodState::NEW));
    FillAndRenderPopularGoods(results);

    return TResultValue();
}

TResultValue TMarketHowMuchRequestImpl::HandleEmptySerp()
{
    LOG(DEBUG) << "No resuts in market. Search in yandex" << Endl;
    Ctx.RenderHowMuchEmptySerp();
    return TResultValue();
}

bool TMarketHowMuchRequestImpl::NeedToShowBeruAdv(const TVector<TReportResponse::TResult>& results) const
{
    if (Ctx.GetCalledFrom() == TStringBuf("hollywood")) {
        // hollywood scenario currently don't support suggests with form_updates to vins scenarios.
        return false;
    }
    if (!Ctx.GetExperiments().AdvBeruScenarioInHowMuch()) {
        return false;
    }
    if (Ctx.GetHowMuchScenarioCtx().FirstGalleryWasShown()) {
        return false;
    }
    for (const auto& result : results) {
        if (result.HasBlueOffer()) {
            return true;
        }
    }
    return false;
}

void TMarketHowMuchRequestImpl::FillAndRenderPopularGoods(const TVector<TReportResponse::TResult>& results)
{
    auto count = Min(MAX_SEARCH_RESULT_GALLERY_COUNT, results.size());
    for (size_t i = 0; i < count; i++) {
        const auto& result = results[i];
        switch (result.GetType()) {
            case TReportResponse::TResult::EType::MODEL: {
                Ctx.AddPopularGoodModel(result.GetModel(), i + 1, result.HasBlueOffer());
                break;
            }
            case TReportResponse::TResult::EType::OFFER: {
                Ctx.AddPopularGoodOffer(result.GetOffer(), i + 1, result.HasBlueOffer());
                break;
            }
            case TReportResponse::TResult::EType::NONE: {
                Y_ASSERT(false);
            }
        }
    }
    Ctx.RenderHowMuchPopularGoods(CurrentRequest, NeedToShowBeruAdv(results));
    Ctx.SetPopularGoodPrices(GetPopularGoodPrices(results));
    Ctx.SetCurrency(GetPopularGoodsCurrency(results));
}

NSc::TValue TMarketHowMuchRequestImpl::GetPopularGoodPrices(const TVector<TReportResponse::TResult>& popularGoodResults)
{
    Y_ASSERT(!popularGoodResults.empty());
    NSc::TValue prices;
    prices["avg"] = 0;
    prices["min"] = 0;

    TPrice min = INT64_MAX;
    TVector<TPrice> avgs;
    for (const auto& result : popularGoodResults) {
        switch (result.GetType()) {
            case TReportResponse::TResult::EType::MODEL: {
                auto model = result.GetModel();
                auto value = model.GetMinPrice();
                if (min > value) {
                    min = value;
                }
                // баг с бильярдными столами - у оффре только цена "от", нет средней
                if (model.HasAvgPrice()) {
                    avgs.push_back(model.GetAvgPrice());
                } else {
                    avgs.push_back(value); // других вариантов все равно нет
                }
                break;
            }
            case TReportResponse::TResult::EType::OFFER: {
                auto offer = result.GetOffer();
                auto value = offer.GetPrice();
                if (value == 0) {
                    value = offer.GetMinPrice();
                }
                if (min > value) {
                    min = value;
                }
                avgs.push_back(value);
                break;
            }
            case TReportResponse::TResult::EType::NONE: {
                Y_ASSERT(false);
            }
        }
    }
    Sort(avgs.begin(), avgs.end());

    prices["min"] = min;
    auto middle = avgs.size() / 2;
    // Убрать вывод копеек в средней цене
    prices["avg"] = (ui64)(avgs.size() % 2 == 1
        ? avgs[middle]
        : (avgs[middle - 1] + avgs[middle]) / 2);
    if (prices["avg"] < 1) {
        prices["avg"] = 1;
    }

    return prices;
}

TString TMarketHowMuchRequestImpl::GetPopularGoodsCurrency(const TVector<TReportResponse::TResult>& results)
{
    if (results.empty()) {
        return TString(DEFAULT_CURRENCY);
    }
    const auto& result = results[0];
    switch (result.GetType()) {
        case TReportResponse::TResult::EType::MODEL:
            return TString(result.GetModel().GetCurrency());
        case TReportResponse::TResult::EType::OFFER:
            return TString(result.GetOffer().GetCurrency());
        default:
            return TString(DEFAULT_CURRENCY);
    }
}

} // namespace NMarket

} // namespace NBASS
