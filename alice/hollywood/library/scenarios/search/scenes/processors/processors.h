#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/search/proto/search.pb.h>

#include <alice/library/search_result_parser/search_result_parser.h>

#include <alice/megamind/protos/analytics/scenarios/search/search.pb.h>
#include <alice/megamind/protos/scenarios/features/search.pb.h>

#include <alice/protos/data/scenario/data.pb.h>

#include <util/generic/vector.h>

namespace NAlice::NHollywoodFw::NSearch {

//
// Aux structure to keep source data and search results
//
struct TSearchResult {
    NData::TScenarioData ScenarioRenderCard;
    NHollywood::TSearchRenderProto RenderArgs;
    NScenarios::TSearchFeatures SearchFeatures;
    NScenarios::TAnalyticsInfo::TObject AiObject;
};

//
// Interface for search processors
//
class ISearchProcessor {
public:
    enum class EProcessorRet {
        Unknown,    // This processor don't know how to handle this call, goto next processor
        Success,    // This processor succesfully parse datasource and ready to answer
        OldFlow,    // This processor knows this source but old flow scenario must be used to make an answer
        Irrelevant  // This processor succesfully parse datasource and ready to answer with irrelevant response
    };

    ISearchProcessor(const TStringBuf& name);
    virtual ~ISearchProcessor() = default;
    virtual EProcessorRet ProcessSearchObject(const TRunRequest& runRequest,
                                              const TSearchResultParser& searchInfo,
                                              TSearchResult& results) const = 0;

    const TString& GetName() const {
        return Name_;
    }
private:
    TString Name_;
};

//
// Main handler for all search processors
//
class TSearchProcessorInstance {
public:
    static TSearchProcessorInstance& Instance();

    // Register new search processor
    template <class TObject>
    void Register() {
        Processors_.emplace_back(std::make_unique<TObject>());
    }

    // Try to handle search request and build output answer
    ISearchProcessor::EProcessorRet ProcessSearchObject(const TRunRequest& runRequest,
                                                        const TSearchResultParser& searchInfo,
                                                        TSearchResult& results);

    static NScenarios::TAnalyticsInfo::TObject CreateAiLog(const TSearchResultParser& searchInfo);
private:
    TVector<std::unique_ptr<ISearchProcessor>> Processors_;
};

} // namespace NAlice::NHollywoodFw::NSearch
