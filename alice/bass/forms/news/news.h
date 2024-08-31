#pragma once

#include "newsdata.h"
#include <alice/bass/forms/geodb.h>
#include <alice/bass/forms/vins.h>

#include <alice/bass/libs/globalctx/globalctx.h>

#include <dict/libs/lemmer_proxy/proxy.h>

#include <library/cpp/text_processing/tokenizer/tokenizer.h>

namespace NBASS {
namespace NNews {

/**
 * News form.
 *
 * @link https://st.yandex-team.ru/ASSISTANT-43
 */
class TNewsFormHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};


class TTextComparer {
public:
    TTextComparer();

    static TStringBuf GetFirstSentence(TStringBuf text);

    float GetSimilarity(TStringBuf title, TStringBuf text) const;

private:
    void SplitToWords(TStringBuf text, TVector<TString>& result) const;

private:
    NTextProcessing::NTokenizer::TTokenizer Tokenizer;
};

const NDict::NLemmerProxy::TLemmerProxy* InitLemmer();

// -------------------------------------------------------------------- Requests

class TRequestType {
public:
    explicit TRequestType(TContext& ctx);
    virtual ~TRequestType() = default;

    void AddDivCard();
    void AddPostNewsPhraseId();
    void AddAnalyticsNewsType(const TString& errorType);

    virtual TResultValue Process() = 0;
    virtual TStringBuf GetNewsType() const = 0;
    bool IsContinuousRequest() const;

protected:
    void CreateSlot(NSc::TValue json);
    void AddContinuousAttention() const;
    void AddSuggests() const;
    void AddTurboIcon(NSc::TValue* dst) const;
    void SaveRequestTime(NSc::TValue& dst) const;
    void SetNoIcon(NSc::TValue* dst) const;
    float GetExpValueByPrefix(TStringBuf prefix, float def) const;

protected:
    TContext& Ctx;
    const NNews::TNewsData& NewsData;
    const NNews::TIssue& Issue;
    const NNews::TRubric& Rubric;
    const size_t MaxNewsCount;

private:
    size_t GetMaxNewsCount() const;
    bool IsPostrollableRequest(const NSc::TValue& mementoValue) const;
};


class TSearchRequest: public TRequestType {
private:
    const TString Text;
    const size_t Offset;
    const size_t NewsCount;

private:
    size_t GetWizardNewsOffset() const;
    TResultValue MakeWebRequest(NSc::TValue& result) const;

public:
    TSearchRequest(TContext& ctx, TStringBuf text);

    TResultValue Process() override;
    TStringBuf GetNewsType() const override { return NEWS_WIZARD_TYPE; };
};

class TNewsApiRequest: public TRequestType {
protected:
    TVector<TString> GetExcludeIdsFromState() const;
    void AddExcludeNewsIds(NSc::TValue& news) const;
    TString BuildExcludeIdsCgi() const;

    virtual TResultValue AddAuxData(const NSc::TValue& newsResponse, NSc::TValue& newsResult) const;
    virtual TResultValue ParseNewsJson(const NSc::TValue& newsResponse, NSc::TValue& result) const;
    virtual TResultValue PrepareRequest(TCgiParameters& cgi) const;
    virtual TResultValue OnFetched(const NSc::TValue& /*newsResponse*/) { return TResultValue(); }

public:
    TNewsApiRequest(TContext& ctx);
    TNewsApiRequest(TContext& ctx, const TString& apiPreset);
    TNewsApiRequest(TContext& ctx, const TString& method, const TString& apiPreset);

    TResultValue Process() override;
    TStringBuf GetNewsType() const override;

private:
    const TString ApiPreset;
    const TString Method;
};


class TGeoApiRequest final : public TNewsApiRequest {
private:
    TRequestedGeo Geo;

private:
    TResultValue AddAuxData(const NSc::TValue& newsResponse, NSc::TValue& newsResult) const override;
    TResultValue PrepareRequest(TCgiParameters& cgi) const override;
    TResultValue OnFetched(const NSc::TValue& newsResponse) override;

public:
    TGeoApiRequest(TContext& ctx, TRequestedGeo geo);

    TStringBuf GetNewsType() const override { return NEWS_GEO_TYPE; };
};


class TPersonalRequest final : public TNewsApiRequest {
private:
    TString YandexUID;

private:
    TResultValue AddAuxData(const NSc::TValue& newsResponse, NSc::TValue& newsResult) const override;
    TResultValue PrepareRequest(TCgiParameters& cgi) const override;

public:
    TPersonalRequest(TContext& ctx, const TString& yandexUID);

    TStringBuf GetNewsType() const override { return NEWS_PERSONAL_TYPE; };
};


class TSmiRequest final : public TNewsApiRequest {
public:
    TSmiRequest(TContext& ctx, const NNews::TSmi smi);

    TResultValue Process() override;
    TStringBuf GetNewsType() const override { return NEWS_SMI_TYPE; };

private:
    TResultValue AddAuxData(const NSc::TValue& newsResponse, NSc::TValue& newsResult) const override;
    TResultValue ParseNewsJson(const NSc::TValue& newsResponse, NSc::TValue& result) const override;
    TResultValue PrepareRequest(TCgiParameters& cgi) const override;

private:
    const NNews::TSmi Smi;
};

// ---------------------------------------------------------------- Requests end


// Context, slots, newsData and rus rubric are needed almost in every method. This class should help to refactor.
class TNewsHandlerHelper {
public:
    explicit TNewsHandlerHelper(TContext& ctx);

    THolder<TRequestType> ProcessGeoRequest(TRequestedGeo geo) const;
    THolder<TRequestType> MakeDefaultRequest() const;
    THolder<TSearchRequest> MakeSearchRequest() const;
    THolder<TSmiRequest> MakeSmiRequest() const;
    THolder<TSmiRequest> MakeSmiRubricRequest() const;
    bool IsUnknownUserRegion() const;
    bool ShouldAskWizardForCovidNews() const;
    bool ShouldAskWizardForFunnyNews() const;
    bool ShouldAskWizardForSmiNews() const;

    void ProcessResult();

    const TString& GetTopicValue() const {
        return TopicValue;
    }

private:
    void CheckSpeakerSpeedDirective() const;
    void SkipRepeatableNewsTitles(float threshold);
    void SkipRepeatedNews();
    // Personal news.
    int GetPersonalActionsThreshold() const;
    bool IsSmiRequest() const;
    bool IsSmiRubricRequest() const;
    TSmi ChooseSmi(const TVector<TSmi>& smis) const;
    bool IsPersonalNewsAllowed() const;
    bool IsUserWarm() const;
    bool HasGeoInfoInAPI(const TRequestedGeo& geo) const;
    bool HasRubricInfoInAPI() const;

private:
    TContext& Ctx;
    const TContext::TSlot* SlotSmi;
    const TContext::TSlot* SlotTopic;
    const TContext::TSlot* SlotWhere;
    const NNews::TNewsData& NewsData;
    const NNews::TIssue& Issue;
    const NNews::TRubric& Rubric;
    const NSc::TValue MementoValue;
    const bool ForeignRequest;
    const bool IsSpeaker;
    TString YandexUID;
    TString TopicValue;
};

} // namespace NNews
} // namespace NBASS
