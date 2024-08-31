#pragma once

#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/library/geo/user_location.h>
#include <alice/hollywood/library/scenarios/search/proto/search.pb.h>
#include <alice/hollywood/library/scenarios/search/resources/resources.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/langs/langs.h>

#include <search/alice/serp_summarizer/runtime/proto/summarizer.pb.h>


namespace NAlice::NHollywood::NSearch {

using TSummarizationRequest = ::NSearch::NAlice::NSerpSummarizer::TSummarizerInput;

struct TSuggest {
    const TStringBuf Type;
    bool AutoAction;
    const NJson::TJsonValue Data;
};

class TSearchContext {
public:
    TSearchContext(const TScenarioRunRequestWrapper& runRequest, TRunResponseBuilder& builder, TContext& ctx,
                   const NScenarios::TRequestMeta& meta, IRng& rng, const NScenarios::TAnalyticsInfo* analyticsInfo = nullptr);

    TResponseBodyBuilder& GetBodyBuilder() {
        return BodyBuilder;
    }

    TNlgData& GetNlgData() {
        return NlgData;
    }

    TSearchState& GetState() {
        return State;
    }

    IRng& GetRng() {
        return Rng;
    }

    TNlgWrapper& GetNlg() {
        return Nlg;
    }

    const TVector<TSuggest>& GetSuggests() {
        return Suggests;
    }

    bool HasPostroll = false;

    const TSearchScenarioResources& GetResources() const;
    const TScenarioRunRequestWrapper& GetRequest() const;
    NScenarios::IAnalyticsInfoBuilder& GetAnalyticsInfoBuilder();
    TRTLogger& GetLogger();
    TStringBuf GetLangName() const;
    const NScenarios::TRequestMeta& GetRequestMeta() const;
    bool IsPornoQuery() const;
    bool IsRecipeQuery() const;
    bool IsSerpSupported() const;
    bool IsSuggestSupported() const;

    void AddAttention(const TStringBuf attention);
    void SetResultSlot(const TStringBuf type, const NJson::TJsonValue& data, bool setIntent = true);

    void AddRenderedCard(NJson::TJsonValue& cardData, const TStringBuf slot = "factoid", const TStringBuf type=TStringBuf("search_fact"));
    void AddSuggest(const TStringBuf attention, bool autoAction = false, const NJson::TJsonValue& data = NJson::TJsonValue(NJson::JSON_NULL));
    void RenderSuggest(const TSuggest suggest);
    void AddRelatedSuggest(const TString& text);
    void AddResult();
    void ReturnIrrelevant(bool hardIrrelevant = false, bool addHandoffPromo = false);
    void ReturnPushNotSuccessful();

    void AddAction(const TString& actionId, NScenarios::TFrameAction&& action);

    void AddTypedSelectedFactAnalytics(const TString& id, const TString& name, const TString& description, const NJson::TJsonValue& fact);

    void SetShouldListen(bool shouldListen);

    NNlg::TRenderPhraseResult RenderPhrase(const TStringBuf phraseName, const TStringBuf templateName=TStringBuf("search"));
    TString GenerateSearchUri() const;
    TString GenerateSearchUri(const TStringBuf query, TCgiParameters cgi = {}) const;

    TString GetTaggerQuery() const;
    TString GetQuery() const;

    NAlice::NScenarios::TSearchFeatures& GetFeatures();
    const NAlice::NScenarios::TInterfaces& GetInterfaces() const;

    const TMaybe<TSummarizationRequest>& GetSerpSummarizationRequest() const;
    void SetSummarizationRequest(TSummarizationRequest request);
    const TMaybe<THttpProxyRequest>& GetSerpSummarizationAsyncRequest() const;
    void SetSummarizationAsyncRequest(THttpProxyRequest request);
    const TMaybe<TString>& GetSerpSummarizationHostPort() const;
    void SetSummarizationHostPort(const TStringBuf hostPort);

    bool GetIsUsingState() const;
    void SetIsUsingState(bool value);

    void SetFactoidSource(const TString& source);
    void SetIntent(const TString& intent);
    void AddPushPromo();
    void AddPushPromoButton(const TString& attentionName);
    void AddDoNothingButton();

private:
    void PrepareContextForNlg();

private:
    const TScenarioRunRequestWrapper& RunRequestWrapper;
    TRunResponseBuilder& Builder;
    TResponseBodyBuilder& BodyBuilder;
    TContext& Ctx;
    TNlgWrapper& Nlg;
    const TStringBuf LangName;
    const NScenarios::TRequestMeta& RequestMeta;

    bool ShouldOpenLink;
    bool ForbidText;
    bool ShouldListen;
    TNlgData NlgData;
    TSearchState State;
    bool IsUsingState;
    IRng& Rng;

    TMaybe<TSummarizationRequest> SummarizationRequest;
    TMaybe<THttpProxyRequest> SummarizationAsyncRequest;
    TMaybe<TString> SummarizationHostPort;

    TVector<TSuggest> Suggests;
};

} // namespace NAlice::NHollywood
