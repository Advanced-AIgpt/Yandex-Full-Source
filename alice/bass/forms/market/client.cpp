#include "client.h"

#include "client/context_logger.h"

#include <alice/bass/forms/market/util/report.h>

#include <util/string/cast.h>
#include <util/string/join.h>

namespace NBASS {

namespace NMarket {

namespace {

constexpr TStringBuf EXP_PREFIX = "/exp";

ui32 GetExpVersionFromKey(TStringBuf key)
{
    if (key.empty() || key[0] != 'v') {
        return 0;
    }
    ui32 result = 0;
    TryFromString(key.SubStr(1), result);
    return result;
}

TString ConvertToString(const TVector<ui64>& ids)
{
    TStringBuilder result;
    for (auto& id: ids) {
        if (!result.empty()) {
            result << ",";
        }
        result << id;
    }
    return result;
}

}

TString TRearFlags::ToString() const
{
    TStringBuilder result;
    for (const auto& kv : Values) {
        if (!result.empty()) {
            result << ";";
        }
        result << kv.first << "=" << kv.second;
    }
    return result;
}

TMarketClient::TMarketClient(TMarketContext& ctx)
    : Logger(MakeHolder<TMarketClientCtxLogger>(ctx))
    , Sources(ctx.GetSources())
    , UserRegion(ToString(ctx.UserRegion()))
    , Experiments(ctx.GetExperiments())
    , MarketType(ctx.GetChoiceMarketType())
    , Clid(static_cast<i32>(ctx.GetMarketClid()))
    , Uuid(ctx.Meta().UUID())
    , Reqid(ctx.RequestId())
    , Ip(ctx.Meta().ClientIP())
    , UseCpmDo(Clid == static_cast<i32>(EClids::CHOICE_GREEN) && !Experiments.DefaultOffer())
    , IsNativeActivation(ctx.IsNative() && ctx.GetChoiceMarketType() == EMarketType::GREEN && ctx.GetScenarioType() == EScenarioType::CHOICE)
    , Ctx(ctx)
{
    LOG(INFO) << "IsNative " << ctx.IsNative() << Endl;
    LOG(INFO) << "ChoiceMarketType " << ctx.GetChoiceMarketType() << Endl;
    LOG(INFO) << "ScenarioType " << ctx.GetScenarioType() << Endl;
    LOG(INFO) << "IsNativeActivation " << IsNativeActivation << Endl;
}

TFormalizerResponse TMarketClient::FormalizeFilterValues(ui64 hid, const TString& filterName, const TString& filterValue)
{
    // Здесь происходит обращение к формализатору. Когда этот метод снова начнёт использоваться, нужно будет
    // обращаться в report-url/yandsearch?place=formalize_gl, и допилить плейс, чтоб можно было передавать
    // названия параметров.
    NSc::TValue query;
    query["category_id"] = hid;
    query["title"] = "";
    query["all_params_multival"].SetBool(true);
    NSc::TValue ymlParam = query["yml_param"].SetArray().Push();
    // todo MALISA-183 баг формализации фильтров из булева блока
    if (filterName != TStringBuf("boolean_block")) {
        ymlParam["name"] = filterName;
    }
    ymlParam["value"] = filterValue;
    return MakeFormalizerRequest(query);
}

TFormalizerResponse TMarketClient::MakeFormalizerRequest(const NSc::TValue& data)
{
    TCgiParameters cgi;
    cgi.InsertEscaped(TStringBuf("q"), data.ToJson());
    NHttpFetcher::TResponse::TRef response = MakeRequest(Sources.MarketFormalizer(), cgi)->Fetch()->Wait();

    TFormalizerResponse r = TFormalizerResponse(response);
    return r;
}

TReportRequest TMarketClient::MakeReportRequest(
    TCgiParameters& cgi,
    TRearFlags& rearFlags,
    const TStringBuf place,
    TMaybe<EMarketType> optionalMarketType)
{
    cgi.InsertUnescaped(TStringBuf("place"), place);

    TString rearValue = rearFlags.ToString();
    if (!rearValue.empty()) {
        cgi.InsertUnescaped(TStringBuf("rearr-factors"), rearValue);
    }

    EMarketType marketType = optionalMarketType.Defined() ? optionalMarketType.GetRef() : MarketType;
    if (marketType == EMarketType::BLUE) {
        cgi.ReplaceUnescaped(TStringBuf("rgb"), TStringBuf("BLUE"));
        cgi.InsertUnescaped(TStringBuf("offer-shipping"), TStringBuf("delivery"));
        cgi.InsertUnescaped(TStringBuf("use-multi-navigation-trees"), TStringBuf("1"));
        cgi.EraseAll(TStringBuf("cpmdo"));
    }

    return TReportRequest(MakeRequest(GetReportSource(marketType, place), cgi), place, marketType);
}

TSourceRequestFactory TMarketClient::GetReportSource(EMarketType marketType, TStringBuf place)
{
    switch (marketType) {
        case EMarketType::BLUE: {
            return place == TStringBuf("prime")
                ? Sources.MarketBlueHeavy()
                : Sources.MarketBlue();
        }
        case EMarketType::GREEN: {
            return place == TStringBuf("prime")
                ? Sources.MarketHeavy()
                : Sources.Market();
        }
    }
}

TCgiParameters TMarketClient::GetBaseReportCgiParams() const
{
    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("regset"), TStringBuf("2"));
    cgi.InsertUnescaped(TStringBuf("rids"), UserRegion);
    cgi.InsertUnescaped(TStringBuf("alice"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("client"), TStringBuf("alice"));
    cgi.InsertUnescaped(TStringBuf("cpmdo"), UseCpmDo ? TStringBuf("1") : TStringBuf("0"));
    cgi.InsertUnescaped(TStringBuf("pp"), ToString(PpCgiParam));
    cgi.InsertUnescaped(TStringBuf("onstock"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("local-offers-first"), TStringBuf("0"));
    cgi.InsertUnescaped(TStringBuf("adult"), TStringBuf("0"));
    cgi.InsertUnescaped(TStringBuf("show-urls"), TStringBuf("external,encryptedmodel"));
    cgi.InsertUnescaped(TStringBuf("pof"), FormatPof(Clid));
    cgi.InsertUnescaped(TStringBuf("uuid"), Uuid);
    cgi.InsertUnescaped(TStringBuf("wprid"), Reqid);
    cgi.InsertUnescaped(TStringBuf("ip"), Ip);
    cgi.InsertUnescaped(TStringBuf("alice-native"), IsNativeActivation ? TStringBuf("1") : TStringBuf("0"));
    cgi.InsertUnescaped(TStringBuf("allow-collapsing"), TStringBuf("1"));
    return cgi;
}

TReportRequest TMarketClient::MakeReportRequest(
    TCgiParameters& cgi,
    const TStringBuf place,
    TMaybe<EMarketType> optionalMarketType)
{
    TRearFlags rearFlags;
    return MakeReportRequest(cgi, rearFlags, place, optionalMarketType);
}

NHttpFetcher::TRequestPtr TMarketClient::MakeRequest(TSourceRequestFactory source, const TCgiParameters& cgi)
{
    NHttpFetcher::TRequestPtr h = source.Request();
    h->AddCgiParams(cgi);
    h->AddHeader("X-Market-Req-ID", Ctx.MarketRequestId());
    Logger->Log(TStringBuilder() << "Request: " << h->Url());

    return h;
}

namespace {
    const unsigned WATCHES = 91259;
    const unsigned SEAT_COVERINGS = 90465;
    const unsigned FAUCETS = 91610;
    const unsigned DESKTOPS = 91011;
    const unsigned WATCHES_AND_ACCESSORIES = 15064473;
    const unsigned MATRESSES = 1003092;
    const unsigned MOBILE_CASES = 91498;
}

/*
 * Запросы к этим категориям падают по таймауту
 * для них приходится добавлять prun-count, чтобы уменьший время ответа репорта
 */
bool TMarketClient::IsBigCategory(const TCategory& category)
{
    return EqualToOneOf(category.GetHid(), MOBILE_CASES);
}

bool TMarketClient::IsVeryBigCategory(const TCategory& category)
{
    return EqualToOneOf(category.GetHid(),
                        WATCHES,
                        SEAT_COVERINGS,
                        FAUCETS,
                        DESKTOPS,
                        WATCHES_AND_ACCESSORIES,
                        MATRESSES);
}

unsigned TMarketClient::GetTextlessPrunCount(const TCategory& category)
{
    if (IsVeryBigCategory(category)) {
        return 1000UL;
    } else if (IsBigCategory(category)) {
        return 5000UL;
    }
    return 20000UL;
}


TReportRequest TMarketClient::MakeFilterRequestAsync(
    const TStringBuf text,
    const TStringBuf suggestText,
    const TCategory& category,
    const TVector<i64>& fesh,
    const NSc::TValue& price,
    const TCgiGlFilters& glFilters,
    const TRedirectCgiParams& redirectParams,
    bool allowRedirects,
    TMaybe<EMarketType> optionalMarketType)
{
    TCgiParameters cgi = GetBaseReportCgiParams();
    if (!text.empty()) {
        cgi.InsertEscaped(TStringBuf("text"), text);
        if (IsBigCategory(category)) {
            cgi.InsertUnescaped(TStringBuf("prun-count"), TStringBuf("10000"));
        }
    } else {
        cgi.InsertUnescaped(TStringBuf("prun-count"), ToString(GetTextlessPrunCount(category)));
    }
    if (!suggestText.empty()) {
        cgi.InsertEscaped(TStringBuf("suggest_text"), suggestText);
    }

    cgi.InsertUnescaped(TStringBuf("hid"), ToString(category.GetHid()));
    if (category.DoesNidExist()) {
        cgi.InsertUnescaped(TStringBuf("nid"), ToString(category.GetNid()));
    }
    cgi.InsertUnescaped(TStringBuf("use-default-offers"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("filterList"), TStringBuf("all"));
    SetRedirectModeCgiParam(allowRedirects, cgi);
    cgi.InsertUnescaped(TStringBuf("numdoc"), TStringBuf("12"));
    if (!Experiments.WithoutShopsSearch()) {
        for (auto& id: fesh) {
            cgi.InsertUnescaped(TStringBuf("fesh"), ToString(id));
        }
    }

    SetPriceCgiParams(price, cgi);
    SetRedirectCgiParams(redirectParams, cgi);
    AddGlFilters(glFilters, cgi);

    TRearFlags rears;
    return MakeReportRequest(cgi, rears, TStringBuf("prime"), optionalMarketType);
}

TReportResponse TMarketClient::MakeFilterRequest(
    const TStringBuf text,
    const TStringBuf suggestText,
    const TCategory& category,
    const TVector<i64>& fesh,
    const NSc::TValue& price,
    const TCgiGlFilters& glFilters,
    const TRedirectCgiParams& redirectParams,
    bool allowRedirects,
    TMaybe<EMarketType> optionalMarketType)
{
    return MakeFilterRequestAsync(
        text, suggestText, category, fesh, price, glFilters, redirectParams, allowRedirects, optionalMarketType).Wait();
}

TReportResponse TMarketClient::FormalizeFilterValues(ui64 hid, const TStringBuf query)
{
    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("hid"), ToString(hid));
    cgi.InsertEscaped(TStringBuf("text"), query);
    return MakeReportRequest(cgi, TStringBuf("formalize_gl")).Wait();
}

TReportRequest TMarketClient::MakeSearchRequestAsync(
    const TStringBuf text,
    const NSc::TValue& price,
    TMaybe<EMarketType> optionalMarketType,
    bool allowRedirects,
    const TVector<i64>& fesh,
    const TRedirectCgiParams& redirectParams,
    const TRearFlags& rearFlags)
{
    TCgiParameters cgi = GetBaseReportCgiParams();
    cgi.InsertEscaped(TStringBuf("text"), text);
    SetRedirectModeCgiParam(allowRedirects, cgi);
    cgi.InsertUnescaped(TStringBuf("allow-collapsing"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("numdoc"), TStringBuf("12"));
    cgi.InsertUnescaped(TStringBuf("use-default-offers"), TStringBuf("1"));
    SetPriceCgiParams(price, cgi);
    SetRedirectCgiParams(redirectParams, cgi);

    TRearFlags rears = rearFlags;
    rears.Add(TStringBuf("market_alice_category_redirect_threshold"), TStringBuf("0.66"));
    if (!Experiments.WithoutParametricSearch()) {
        rears.Add(TStringBuf("market_disable_parametric_search_for_white_except_parametric_specification"), TStringBuf("0"));
    }

    for (const auto& id: fesh) {
        cgi.InsertUnescaped(TStringBuf("fesh"), ToString(id));
    }
    return MakeReportRequest(cgi, rears, TStringBuf("prime"), optionalMarketType);
}

TReportResponse TMarketClient::MakeSearchRequest(
    const TStringBuf text,
    const NSc::TValue& price,
    TMaybe<EMarketType> optionalMarketType,
    bool allowRedirects,
    const TVector<i64>& fesh,
    const TRedirectCgiParams& redirectParams)
{
    return MakeSearchRequestAsync(text, price, optionalMarketType, allowRedirects, fesh, redirectParams).Wait();
}

TReportRequest TMarketClient::MakeDefinedDocsRequestAsync(
    const TStringBuf text,
    const TVector<ui64>& skus,
    const NSc::TValue& price,
    TMaybe<EMarketType> optionalMarketType,
    bool allowRedirects)
{
    TCgiParameters cgi = GetBaseReportCgiParams();
    cgi.InsertEscaped(TStringBuf("text"), text);
    cgi.InsertUnescaped(TStringBuf("market-sku"), ConvertToString(skus));
    SetRedirectModeCgiParam(allowRedirects, cgi);
    cgi.InsertUnescaped(TStringBuf("allow-collapsing"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("numdoc"), TStringBuf("12"));
    cgi.InsertUnescaped(TStringBuf("use-default-offers"), TStringBuf("1"));
    SetPriceCgiParams(price, cgi);

    TRearFlags rears;
    rears.Add(TStringBuf("market_alice_category_redirect_threshold"), TStringBuf("0.66"));

    return MakeReportRequest(cgi, rears, TStringBuf("prime"), optionalMarketType);
}

TReportResponse TMarketClient::MakeDefinedDocsRequest(
    const TStringBuf text,
    const TVector<ui64>& skus,
    const NSc::TValue& price,
    TMaybe<EMarketType> optionalMarketType,
    bool allowRedirects)
{
    return MakeDefinedDocsRequestAsync(text, skus, price, optionalMarketType, allowRedirects).Wait();
}

TReportResponse TMarketClient::MakeSearchModelRequest(
    TModelId modelId,
    const TCgiGlFilters& glFilters,
    const TRedirectCgiParams& redirectParams,
    bool withSpecs)
{
    // http://warehouse-report.vs.market.yandex.net:17051/yandsearch?
    //          rids=213&place=modelinfo&hyperid=10477708&bsformat=2
    TCgiParameters cgi = GetBaseReportCgiParams();
    cgi.InsertUnescaped(TStringBuf("use-default-offers"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("hyperid"), ToString(modelId));
    cgi.InsertUnescaped(TStringBuf("bsformat"), TStringBuf("2"));
    if (withSpecs) {
        cgi.InsertUnescaped(TStringBuf("show-models-specs"), TStringBuf("friendly"));
    }
    SetRedirectCgiParams(redirectParams, cgi);
    AddGlFilters(glFilters, cgi);

    return MakeReportRequest(cgi, TStringBuf("modelinfo")).Wait();
}

TReportResponse TMarketClient::MakeSearchOfferRequest(const TStringBuf wareId)
{
    TCgiParameters cgi = GetBaseReportCgiParams();
    cgi.InsertUnescaped(TStringBuf("offerid"), ToString(wareId));
    cgi.InsertUnescaped(TStringBuf("show-urls"), TStringBuf("external,cpa,showPhone,geo,geoShipping,decrypted"));
    cgi.InsertUnescaped(TStringBuf("regset"), TStringBuf("2"));

    return MakeReportRequest(cgi, TStringBuf("offerinfo")).Wait();
}

TReportResponse TMarketClient::MakeSimilarCategoriesRequest(const TString& text)
{
    TCgiParameters cgi = GetBaseReportCgiParams();
    cgi.InsertEscaped(TStringBuf("text"), text);
    cgi.InsertUnescaped(TStringBuf("cvredirect"), TStringBuf("0"));
    cgi.InsertUnescaped(TStringBuf("allow-collapsing"), TStringBuf("1"));

    return MakeReportRequest(cgi).Wait();
}

TReportResponse TMarketClient::MakeCategoryRequest(
    const TCategory& category,
    const TCgiGlFilters& glFilters,
    const TRedirectCgiParams& redirectParams,
    const TStringBuf text,
    const TStringBuf suggestText)
{
    TCgiParameters cgi = GetBaseReportCgiParams();
    if (!text.empty()) {
        cgi.InsertEscaped(TStringBuf("text"), text);
        if (IsBigCategory(category)) {
            cgi.InsertUnescaped(TStringBuf("prun-count"), TStringBuf("10000"));
        }
    } else {
        cgi.InsertUnescaped(TStringBuf("prun-count"), ToString(GetTextlessPrunCount(category)));
    }
    if (!suggestText.empty()) {
        cgi.InsertEscaped(TStringBuf("suggest_text"), suggestText);
    }
    cgi.InsertUnescaped(TStringBuf("cvredirect"), TStringBuf("0"));
    cgi.InsertUnescaped(TStringBuf("hid"), ToString(category.GetHid()));
    cgi.InsertUnescaped(TStringBuf("allow-collapsing"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("numdoc"), TStringBuf("12"));
    cgi.InsertUnescaped(TStringBuf("use-default-offers"), TStringBuf("1"));
    SetRedirectCgiParams(redirectParams, cgi);
    AddGlFilters(glFilters, cgi);

    return MakeReportRequest(cgi).Wait();
}

/////////////////////////////////// TMdsClient

TMdsClient::TMdsClient(const TSourcesRequestFactory& sources)
    : Logger(MakeHolder<TMarketClientLogger>())
    , Sources(sources)
{
}

NHttpFetcher::THandle::TRef TMdsClient::MakeRequestAsync(TSourceRequestFactory source)
{
    NHttpFetcher::TRequestPtr h = source.Request();
    Logger->Log(TStringBuilder() << "Request: " << h->Url());
    return h->Fetch();
}

NHttpFetcher::TResponse::TRef TMdsClient::MakeRequest(TSourceRequestFactory source)
{
    return MakeRequestAsync(source)->Wait();
}

TMaybe<THashSet<TString>> TStopWordsGetter::MakeStopWordsRequest()
{
    const auto source = Sources.MarketMds(TStringBuf("/words/content"));
    const auto response = MakeRequest(source);
    return THashSetResponse<TString>(response).Data;
}

TMaybe<TMdsCategoriesGetterBase::TData> TMdsCategoriesGetterBase::MakeMdsCategoriesRequest(ECategoriesType type)
{
    TStringBuf path;
    switch(type) {
        case ECategoriesType::Stop:
            path = TStringBuf("/categories/content");
            break;
        case ECategoriesType::Denied:
            path = TStringBuf("/categories/denied");
            break;
        case ECategoriesType::Allowed:
            path = TStringBuf("/categories/allowed");
            break;
        case ECategoriesType::AllowedOnMarket:
            path = TStringBuf("/categories/allowed_on_market");
            break;
        case ECategoriesType::DeniedOnMarket:
            path = TStringBuf("/categories/denied_on_market");
            break;
    }
    const auto source = Sources.MarketMds(path);
    const auto expSource = Sources.MarketMds(TString(EXP_PREFIX) + path);
    const auto request = MakeRequestAsync(source);
    const auto expRequest = MakeRequestAsync(expSource);
    const TStringResponse responseString(request->Wait());
    const auto expResponse = expRequest->Wait();
    if (!responseString.Data().Defined()) {
        return Nothing();
    }
    TData result(THashSetResponse<ui64>::ReadFromString(responseString.Data().GetRef()));

    const NSc::TValue data = NSc::TValue::FromJson(expResponse->Data);
    if (data.IsDict()) {
        for (const auto key : data.DictKeys()) {
            if (ui32 expVersion = GetExpVersionFromKey(key)) {
                result.SetExpData(expVersion, THashSetResponse<ui64>::ReadFromString(data.Get(key).GetString()));
            } else {
                LOG(ERR) << TStringBuf("Unexpected key '") << key << TStringBuf("' in mds data for path ") << EXP_PREFIX << path << Endl;
            }
        }
    } else {
        LOG(ERR) << TStringBuf("Malformed mds data for path ") << EXP_PREFIX << path << Endl;
    }
    return result;
}

TMaybe<TPromotions> TPromotionsGetter::MakePromotionsRequest()
{
    const auto source = Sources.MarketMds(TStringBuf("/promotions"));
    const auto response = MakeRequest(source);
    NSc::TValue data = NSc::TValue::FromJson(response->Data);
    if (data.IsNull()) {
        return Nothing();
    }
    return TPromotions::FromJson(data);
}

TMaybe<TPhrasesGetter::TData> TPhrasesGetter::MakePhrasesRequest()
{
    const TStringBuf path = "/phrases";
    const TString expPath = TString(EXP_PREFIX) + path;
    const auto source = Sources.MarketMds(path);
    const auto expSource = Sources.MarketMds(expPath);
    const auto request = MakeRequestAsync(source);
    const auto expRequest = MakeRequestAsync(expSource);
    const auto response = request->Wait();
    const auto expResponse = expRequest->Wait();
    NSc::TValue data = NSc::TValue::FromJson(response->Data);
    if (data.IsNull()) {
        return Nothing();
    }
    TPhrasesGetter::TData result(std::move(data));
    const NSc::TValue expData = NSc::TValue::FromJson(expResponse->Data);
    if (expData.IsDict()) {
        for (const auto key : expData.DictKeys()) {
            if (ui32 expVersion = GetExpVersionFromKey(key)) {
                result.SetExpData(expVersion, expData.Get(key).Clone());
            } else {
                LOG(ERR) << TStringBuf("Unexpected key '") << key << TStringBuf("' in mds data for path ") << expPath << Endl;
            }
        }
    } else {
        LOG(ERR) << TStringBuf("Malformed mds data for path ") << expPath << Endl;
    }
    return result;
}

} // namespace NMarket

} // namespace NBASS
