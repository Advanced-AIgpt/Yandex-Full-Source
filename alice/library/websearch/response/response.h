#pragma once

#include <alice/library/logger/logger.h>

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/web_search_source.pb.h>
#include <alice/protos/websearch/websearch_parts.pb.h>

#include <library/cpp/scheme/scheme.h>

#include <search/begemot/rules/query_factors/proto/query_factors.pb.h>

namespace NAlice {

inline constexpr TStringBuf AH_ITEM_SEARCH_PART_TUNNELLER_RAW_RESPONSE = "search_part_tunneller_raw_response";
inline constexpr TStringBuf AH_ITEM_SEARCH_PART_BG_FACTORS = "search_part_bg_factors";
inline constexpr TStringBuf AH_ITEM_SEARCH_PART_STATIC_BLENDER_FACTORS = "search_part_static_blender_factors";
inline constexpr TStringBuf AH_ITEM_SEARCH_PART_REPORT_GROUPING = "search_part_report_grouping";

class TSearchResponse {
public:
    using TApplyBlenderFactors = TVector<float>;
    using TBgFactors = NBg::NProto::TQueryFactors;
    using TDocs = NSc::TArray;

public:
    TSearchResponse(
        const TString& response,
        const TString& requestActivationId,
        TRTLogger& logger,
        bool initDataSources
    );

    TSearchResponse(
        TMaybe<NWebSearch::TTunnellerRawResponse>&& tunnellerRawResponsePart,
        TMaybe<NWebSearch::TStaticBlenderFactors>&& staticBlenderFactorsPart,
        TMaybe<NWebSearch::TReportGrouping>&& reportGroupingPart,
        TMaybe<NScenarios::TDataSource>&& docsDataSource,
        TMaybe<TBgFactors>&& bgFactorsPart,
        TRTLogger& logger
    );

    const NSc::TValue& Report() const;

    const TApplyBlenderFactors* StaticBlenderFactors() const;

    const TBgFactors* BgFactors() const;

    const NSc::TValue& Data() const;

    const NScenarios::TWebSearchDocs& Docs() const;

    TMaybe<TString> GetTunnellerRawResponse() const;

    const NSc::TArray& GetReportGrouping() const;

    // DataSources
    // Getters
    const NScenarios::TWebSearchDocs& WebSearchDocs() const;
    const NScenarios::TWebSearchDocsRight& WebSearchDocsRight() const;
    const NScenarios::TWebSearchWizplaces& WebSearchWizplaces() const;
    const NScenarios::TWebSearchSummarization& WebSearchSummarization() const;
    const NScenarios::TWebSearchWizard& WebSearchWizard() const;
    const NScenarios::TWebSearchBanner& WebSearchBanner() const;
    const NScenarios::TWebSearchRenderrer& WebSearchRenderrer() const;

    // Initializers (will be removed after tranfering scenario requests to apphost)
    static void InitWebSearchDocs(const TSearchResponse& searchResponse, NScenarios::TWebSearchDocs* result);
    static void InitWebSearchDocsRight(const TSearchResponse& searchResponse, NScenarios::TWebSearchDocsRight* result);
    static void InitWebSearchWizplaces(const TSearchResponse& searchResponse, NScenarios::TWebSearchWizplaces* result);
    static void InitWebSearchSummarization(const TSearchResponse& searchResponse, NScenarios::TWebSearchSummarization* result);
    static void InitWebSearchWizard(const TSearchResponse& searchResponse, NScenarios::TWebSearchWizard* result);
    static void InitWebSearchBanner(const TSearchResponse& searchResponse, NScenarios::TWebSearchBanner* result);
    static void InitWebSearchRenderrer(const TSearchResponse& searchResponse, NScenarios::TWebSearchRenderrer* result);

private:
    void Init(const TString& requestActivationId);

private:
    NSc::TValue Data_;
    TRTLogger& Logger_;
    TMaybe<TApplyBlenderFactors> StaticBlenderFactors_;
    TMaybe<TBgFactors> BgFactors_;

    bool InitDataSources_;
    NScenarios::TWebSearchDocs WebSearchDocs_;
    NScenarios::TWebSearchDocsRight WebSearchDocsRight_;
    NScenarios::TWebSearchWizplaces WebSearchWizplaces_;
    NScenarios::TWebSearchSummarization WebSearchSummarization_;
    NScenarios::TWebSearchWizard WebSearchWizard_;
    NScenarios::TWebSearchBanner WebSearchBanner_;
    NScenarios::TWebSearchRenderrer WebSearchRenderrer_;

    TMaybe<NWebSearch::TTunnellerRawResponse> TunnellerRawResponsePart_;
    TMaybe<NSc::TValue> ReportGrouping_;
};

} // namspece NAlice
