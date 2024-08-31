#pragma once

#include "context.h"
#include "experiments.h"
#include "types.h"
#include "types/model.h"

#include <alice/bass/forms/context/context.h>

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <util/generic/lazy_value.h>
#include <util/string/split.h>
#include <util/string/strip.h>

namespace NBASS {

namespace NMarket {

////////////////////////////////////////////////////////////////////////////////

class TBaseResponse {
public:
    explicit TBaseResponse(const NHttpFetcher::TResponse::TRef response);
    virtual ~TBaseResponse() = default;

    const TError& GetError() const;
    bool HasError() const;

protected:
    TMaybe<TString> RawData;
    TMaybe<TError> Error;
};

class TBaseJsonResponse: public TBaseResponse {
public:
    explicit TBaseJsonResponse(const NHttpFetcher::TResponse::TRef response);

protected:
    TMaybe<NSc::TValue> Data;
};

class TStringResponse: public TBaseResponse {
public:
    explicit TStringResponse(const NHttpFetcher::TResponse::TRef response) : TBaseResponse(response) {}

    const TMaybe<TString>& Data() const { return RawData; }
};

////////////////////////////////////////////////////////////////////////////////

class TReportResponse: public TBaseJsonResponse {
public:
    enum ERedirectType {
        NONE,
        PARAMETRIC,
        MODEL,
        REGION,
        UNKNOWN
    };

    class TBaseRedirect {
    public:
        explicit TBaseRedirect(NSc::TValue data);
        const TString& GetText() const;
        const TString& GetSuggestText() const;
        const TCgiGlFilters& GetGlFilters() const;
        const TRedirectCgiParams& GetCgiParams() const;

    protected:
        NSc::TValue Data;
        TString Text;
        TString SuggestText;
        TCgiGlFilters GlFilters;
        TRedirectCgiParams CgiParams;

    private:
        void SetGlFilters(const NSc::TValue& rawFilters);
    };

    class TParametricRedirect: public TBaseRedirect {
    public:
        explicit TParametricRedirect(NSc::TValue data);

        const TCategory& GetCategory() const;
        const TString& GetReportState() const;
        const TVector<i64>& GetFesh() const;

    private:
        TStringBuf GetCategoryName() const;

        TString ReportState;
        TCategory Category;
        TVector<i64> Fesh;
    };

    class TModelRedirect: public TBaseRedirect {
    public:
        explicit TModelRedirect(NSc::TValue data, EMarketType marketType);
        TModelId GetModelId() const;
        ui64 GetSkuId() const;
        TMaybe<ui64> GetHid() const;

    private:
        TModelId ModelId;
        ui64 SkuId;
        TMaybe<ui64> Hid;
    };

    class TRegionRedirect: public TBaseRedirect {
    public:
        explicit TRegionRedirect(NSc::TValue data);

        i64 GetUserRegion() const;

    private:
        i64 UserRegion;
    };

    /// Универсальный класс для всевозможных редиректов
    class TRedirect : public TBaseRedirect {
    public:
        explicit TRedirect(NSc::TValue data);
        bool HasCategory() const;
        const TCategory& GetCategory() const;
        const TString& GetReportState() const;
        const TVector<i64>& GetFesh() const;
        bool HasUserRegion() const;
        i64 GetUserRegion() const;
        void FillCtx(TMarketContext& ctx) const;

    protected:
        TStringBuf GetCategoryName() const;
        TString ReportState;
        TMaybe<TCategory> Category;
        TVector<i64> Fesh;
        TMaybe<i64> UserRegion;

    private:
        void SetGlFilters(const NSc::TValue& rawFilters);
    };

    class TResult {
    public:
        enum EType {
            MODEL,
            OFFER,
            NONE
        };

        explicit TResult(NSc::TValue);

        EType GetType() const;
        TModel GetModel() const;
        TVector<TOffer> GetModelOffers() const;
        TOffer GetOffer() const;
        const NSc::TArray& GetCategories() const;
        TVector<TWarning> GetWarnings() const;
        bool HasBlueOffer() const;

        const NSc::TValue& GetRawData() const {
            return Data;
        }

    private:
        NSc::TValue Data;
        EType Type;
    };

    explicit TReportResponse(
        const NHttpFetcher::TResponse::TRef response,
        const TStringBuf place,
        EMarketType marketType);
    TReportResponse(const TReportResponse&) = default;
    TReportResponse(TReportResponse&&) = default;

    ERedirectType GetRedirectType() const;
    TParametricRedirect GetParametricRedirect() const;
    TModelRedirect GetModelRedirect() const;
    TRegionRedirect GetRegionRedirect() const;
    TRedirect GetRedirect() const {
        Y_ASSERT(EqualToOneOf(Place, TStringBuf("prime")));
        Y_ASSERT(RedirectType != TReportResponse::ERedirectType::NONE);
        return TRedirect(Data.GetRef());
    }
    TVector<TResult> GetResults() const;
    const NSc::TArray& GetFilters() const;
    const NSc::TValue* GetFilter(const TStringBuf) const;
    const NSc::TArray& GetIntents() const;
    i64 GetTotal() const;
    TCgiGlFilters GetFormalizedCgiGlFilters() const;
    const TFormalizedGlFilters& GetFormalizedGlFilters() const;

private:
    ERedirectType RedirectType;
    TString Place;
    EMarketType MarketType;
    TLazyValue<TFormalizedGlFilters> FormalizedGlFilters;

    TLazyValue<TFormalizedGlFilters> InitFormalizedGlFilters() const;
};

////////////////////////////////////////////////////////////////////////////////

class TFormalizerResponse: public TBaseJsonResponse {
public:
    explicit TFormalizerResponse(const NHttpFetcher::TResponse::TRef response);

    THashMap<TString, TVector<TString>> GetFilters() const;
};

////////////////////////////////////////////////////////////////////////////////

template <typename TItem>
class THashSetResponse: public TBaseResponse {
public:
    explicit THashSetResponse(const NHttpFetcher::TResponse::TRef response)
        : TBaseResponse(response)
    {
        if (RawData.Defined()) {
            Data.ConstructInPlace(ReadFromString(RawData.GetRef()));
        }
    }

    static THashSet<TItem> ReadFromString(TStringBuf data)
    {
        THashSet<TItem> result;
        for (const auto& it : StringSplitter(data).Split('\n').SkipEmpty()) {
            TItem item;
            if (TryFromString<TItem>(StripString(it.Token()), item)) {
                result.insert(item);
            }
        }
        return result;
    }

    TMaybe<THashSet<TItem>> Data;

};

////////////////////////////////////////////////////////////////////////////////

class TMarketClientLogger {
public:
    virtual ~TMarketClientLogger() = default;

    virtual void Log(const TStringBuf msg)
    {
        LOG(INFO) << msg << Endl;
    };
};

////////////////////////////////////////////////////////////////////////////////

class TRearFlags {
public:
    template <class TName, class TValue>
    void Add(TName&& name, TValue&& value)
    {
        Values.emplace(std::forward<TName>(name), std::forward<TValue>(value));
    }

    TString ToString() const;

private:
    THashMap<TString, TString> Values;
};

////////////////////////////////////////////////////////////////////////////////

class TReportRequest {
public:
    TReportRequest(
        NHttpFetcher::TRequestPtr httpRequest,
        TStringBuf place,
        EMarketType marketType);
    TReportRequest(const TReportRequest&) = default;
    TReportRequest(TReportRequest&&) = default;

    TReportResponse Wait();

private:
    NHttpFetcher::TRequestPtr HttpRequest;
    NHttpFetcher::THandle::TRef Handle;
    TString Place;
    EMarketType MarketType;
};

////////////////////////////////////////////////////////////////////////////////

class TMarketClient {
public:
    explicit TMarketClient(TMarketContext& ctx);

    TFormalizerResponse FormalizeFilterValues(ui64 hid, const TString& filterName, const TString& filterValue);
    TReportResponse FormalizeFilterValues(ui64 hid, const TStringBuf query);

    TReportRequest MakeFilterRequestAsync(
        const TStringBuf text,
        const TStringBuf suggestText,
        const TCategory& category,
        const TVector<i64>& fesh,
        const NSc::TValue& price,
        const TCgiGlFilters& glFilters,
        const TRedirectCgiParams& redirectParams,
        bool allowRedirects = false,
        TMaybe<EMarketType> optionalMarketType = Nothing());
    TReportResponse MakeFilterRequest(
        const TStringBuf text,
        const TStringBuf suggestText,
        const TCategory& category,
        const TVector<i64>& fesh,
        const NSc::TValue& price,
        const TCgiGlFilters& glFilters,
        const TRedirectCgiParams& redirectParams,
        bool allowRedirects = false,
        TMaybe<EMarketType> optionalMarketType = Nothing());

    TReportRequest MakeSearchRequestAsync(
        const TStringBuf categoryQuery,
        const NSc::TValue& price = NSc::TValue(),
        TMaybe<EMarketType> optionalMarketType = Nothing(),
        bool allowRedirects = true,
        const TVector<i64>& fesh = {},
        const TRedirectCgiParams& redirectParams = TRedirectCgiParams(),
        const TRearFlags& rearFlags = {});
    TReportResponse MakeSearchRequest(
        const TStringBuf categoryQuery,
        const NSc::TValue& price = NSc::TValue(),
        TMaybe<EMarketType> optionalMarketType = Nothing(),
        bool allowRedirects = true,
        const TVector<i64>& fesh = {},
        const TRedirectCgiParams& redirectParams = TRedirectCgiParams());

    TReportResponse MakeSearchModelRequest(
        TModelId modelId,
        const TCgiGlFilters& glFilters = TCgiGlFilters(),
        const TRedirectCgiParams& redirectParams = TRedirectCgiParams(),
        bool withSpecs = false);
    TReportResponse MakeSearchOfferRequest(const TStringBuf wareId);
    TReportResponse MakeSimilarCategoriesRequest(const TString& categoryQuery);
    TReportResponse MakeCategoryRequest(
        const TCategory& category,
        const TCgiGlFilters& glFilters,
        const TRedirectCgiParams& redirectParams,
        const TStringBuf text,
        const TStringBuf suggestText);

    TReportRequest MakeDefinedDocsRequestAsync(
        const TStringBuf categoryQuery,
        const TVector<ui64>& skus,
        const NSc::TValue& price = NSc::TValue(),
        TMaybe<EMarketType> optionalMarketType = Nothing(),
        bool allowRedirects = true);
    TReportResponse MakeDefinedDocsRequest(
        const TStringBuf categoryQuery,
        const TVector<ui64>& skus,
        const NSc::TValue& price = NSc::TValue(),
        TMaybe<EMarketType> optionalMarketType = Nothing(),
        bool allowRedirects = true);

private:
    TCgiParameters GetBaseReportCgiParams() const;
    void AddFiltersExpFlags(TCgiParameters& cgi, TRearFlags& rearFlags);
    bool IsBigCategory(const TCategory& category);
    bool IsVeryBigCategory(const TCategory& category);
    unsigned GetTextlessPrunCount(const TCategory& category);
    TReportRequest MakeReportRequest(
        TCgiParameters& cgi,
        const TStringBuf place = "prime",
        TMaybe<EMarketType> optionalMarketType = Nothing());
    TReportRequest MakeReportRequest(
        TCgiParameters& cgi,
        TRearFlags& rearFlags,
        const TStringBuf place = "prime",
        TMaybe<EMarketType> optionalMarketType = Nothing());
    TSourceRequestFactory GetReportSource(EMarketType marketType, TStringBuf place);
    NHttpFetcher::TRequestPtr MakeRequest(TSourceRequestFactory source, const TCgiParameters& cgi);
    TFormalizerResponse MakeFormalizerRequest(const NSc::TValue& data);

private:
    THolder<TMarketClientLogger> Logger;
    TSourcesRequestFactory Sources;
    TString UserRegion;
    const TMarketExperiments& Experiments;
    EMarketType MarketType;
    i32 Clid;
    TStringBuf Uuid;
    TStringBuf Reqid;
    TStringBuf Ip;
    bool UseCpmDo;
    bool IsNativeActivation;
    TMarketContext& Ctx;
};

////////////////////////////////////////////////////////////////////////////////

class TMdsClient {
public:
    explicit TMdsClient(const TSourcesRequestFactory& sources);
    virtual ~TMdsClient() = default;

protected:
    NHttpFetcher::THandle::TRef MakeRequestAsync(TSourceRequestFactory source);
    NHttpFetcher::TResponse::TRef MakeRequest(TSourceRequestFactory source);

    THolder<TMarketClientLogger> Logger;
    TSourcesRequestFactory Sources;
};

////////////////////////////////////////////////////////////////////////////////

template <typename T>
class TDataWithExp {
public:
    using TDataUnit = T;

    TDataWithExp(T&& data) : Data(data) {}

    void SetExpData(ui32 expVersion, T&& data) { ExpData[expVersion] = data; }

    const T& GetData() const { return Data; }
    const T& GetData(ui32 expVersion) const
    {
        const auto it = ExpData.find(expVersion);
        if (it != ExpData.cend()) {
            return it->second;
        }
        return Data;
    }

private:
    T Data;
    THashMap<ui32, T> ExpData;
};

////////////////////////////////////////////////////////////////////////////////

class TStopWordsGetter: private TMdsClient {
public:
    using TData = THashSet<TString>;

    explicit TStopWordsGetter(const TSourcesRequestFactory& sources) : TMdsClient(sources) {}

    TMaybe<TData> GetData() { return MakeStopWordsRequest(); }

private:
    TMaybe<THashSet<TString>> MakeStopWordsRequest();
};

////////////////////////////////////////////////////////////////////////////////

enum class ECategoriesType {
    Stop, // Стоп-категории, выдача по этим категориям фильтруется
    Denied, // По этим категориям нам запрещено активироваться при нативной активации
    Allowed, // По этим категориям нам разрешено активироваться при нативной активации
    DeniedOnMarket, // По этим категориям нам запрещено активироваться при нативной активации с "на маркете"
    AllowedOnMarket // По этим категориям нам разрешено активироваться при нативной активации с "на маркете"
};

class TMdsCategoriesGetterBase: private TMdsClient {
public:
    using TData = TDataWithExp<THashSet<ui64>>;

    explicit TMdsCategoriesGetterBase(const TSourcesRequestFactory& sources) : TMdsClient(sources) {}

protected:
    TMaybe<TData> MakeMdsCategoriesRequest(ECategoriesType type);
};

template <ECategoriesType CategoriesType>
class TMdsCategoriesGetter: public TMdsCategoriesGetterBase {
public:
    explicit TMdsCategoriesGetter(const TSourcesRequestFactory& sources) : TMdsCategoriesGetterBase(sources) {}

    TMaybe<TData> GetData() { return MakeMdsCategoriesRequest(CategoriesType); }
};

////////////////////////////////////////////////////////////////////////////////

class TPromotionsGetter: private TMdsClient {
public:
    using TData = TPromotions;

    explicit TPromotionsGetter(const TSourcesRequestFactory& sources) : TMdsClient(sources) {}

    TMaybe<TData> GetData() { return MakePromotionsRequest(); }

private:
    TMaybe<TPromotions> MakePromotionsRequest();
};

////////////////////////////////////////////////////////////////////////////////

class TPhrasesGetter: private TMdsClient {
public:
    using TData = TDataWithExp<NSc::TValue>;

    explicit TPhrasesGetter(const TSourcesRequestFactory& sources) : TMdsClient(sources) {}

    TMaybe<TData> GetData() { return MakePhrasesRequest(); }

private:
    TMaybe<TData> MakePhrasesRequest();
};

////////////////////////////////////////////////////////////////////////////////

} // namespace NMarket

} // namespace NBASS
