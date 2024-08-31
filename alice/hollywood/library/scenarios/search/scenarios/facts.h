#pragma once

#include "base.h"

#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/scenarios/search/utils/serp_helpers.h>
#include <alice/hollywood/library/base_scenario/scenario.h>

#include <search/alice/serp_summarizer/runtime/proto/bin/server.pb.h>

#include <util/generic/map.h>
#include <util/generic/maybe.h>

namespace NAlice::NHollywood::NSearch {

using TSummarizationEndResponse = ::NSearch::NAlice::NSerpSummarizer::TServerResponse;
using TSummarizationRequest = ::NSearch::NAlice::NSerpSummarizer::TSummarizerInput;
using TSummarizationAsyncRequest = ::NSearch::NAlice::NSerpSummarizer::TAsyncResult;

class TSearchFactsScenario : public TSearchScenario {
public:
    using TSearchScenario::TSearchScenario;

    bool Do(const TSearchResult& response) override;
    bool DoObject(const TSearchResult& response);
    bool DoSummarization(const TSearchResult& response);

    bool AddSummarizationAnswer(TScenarioHandleContext& ctx);
private:
    struct TRelatedFact {
        TString Query;
        NJson::TJsonValue SerpData; // Optional (works only with discovery).
    };

    bool AddCalculator(const TSearchResult& searchResult, size_t& pos);
    bool AddCaloriesFact(const TSearchResult& searchResult, size_t& pos);
    bool AddDistanceFact(const TSearchResult& searchResult, size_t& pos);
    bool AddEntityFact(const TSearchResult& searchResult, size_t& pos);
    bool AddObjectAsFact(const TSearchResult& searchResult, size_t& pos);
    bool AddRichFact(const TSearchResult& searchResult, size_t& pos);
    bool AddSportLivescore(const TSearchResult& searchResult, size_t& pos);
    bool AddSportLivescoreDocs(const TSearchResult& docs, size_t& pos);
    bool AddSportLivescoreImpl(const NJson::TJsonValue& data);
    bool AddSportLivescoreWizplaces(const TSearchResult& wizplaces);
    bool AddSuggestFact(const TSearchResult& searchResult, size_t& pos);
    bool AddSuggestFactImpl(const NJson::TJsonValue& factoidData);
    bool AddTableFact(const TSearchResult& searchResult, size_t& pos);
    bool AddTimeDifference(const TSearchResult& searchResult, size_t& pos);
    bool AddUnitsConverter(const TSearchResult& searchResult, size_t& pos);
    bool AddZipCode(const TSearchResult& searchResult, size_t& pos);
    bool AddRelatedFactFallback();
    bool AddFactoid(const TSearchResult& searchResult, size_t& pos);
    void AddFactoidPhone(const NJson::TJsonValue& factoidData, NJson::TJsonValue& factoid, bool addSuggest);
    void AddRelatedFactPromo(const NJson::TJsonValue& factoidData, NJson::TJsonValue& factoid) const;
    bool ShouldAnswerWithSummarization(const TSearchResult& searchResult) const;
    void AddSummarizationRequest(const TSearchResult& searchResult, TRTLogger& logger);
    bool AddSummarizationSyncRequest(const TSearchResult& searchResult, TRTLogger& logger);
    bool AddSummarizationAsyncRequest(const TSearchResult& searchResult, TRTLogger& logger);
    bool CheckLiveScoreDoc(const ::google::protobuf::Struct& doc);
    bool ProcessBannedWizards(const TSearchResult& searchResult, size_t& pos);
    bool ResetFeatures();

    const NJson::TJsonValue& GetVoiceInfo(const NJson::TJsonValue& snippet);
    NJson::TJsonValue GetFilteredVoiceInfo(const NJson::TJsonValue& snippet);

    bool IsBadRelatedFactQuery(const TStringBuf& query) const;
    static TVector<TRelatedFact> GetRelatedFactsFromFactoid(const NJson::TJsonValue& factoid);
    static TVector<TRelatedFact> GetRelatedFactsFromDiscovery(const TSearchResult& response);
};
TMaybe<NJson::TJsonValue> FindFactoidInDocsRight(const TSearchResult& searchResult, const TStringBuf snippetType,
                                                 size_t maxPos, ESnippetSection section, size_t& pos);

TMaybe<NJson::TJsonValue> FindFactoidInWizplaces(const TSearchResult& searchResult, const TStringBuf snippetType);

TMaybe<NJson::TJsonValue> FindFactoidInDocs(const TSearchResult& searchResult, const TStringBuf snippetType,
                                            size_t maxPos, ESnippetSection section, size_t& pos);
} // namespace NAlice::NHollywood::NSearch
