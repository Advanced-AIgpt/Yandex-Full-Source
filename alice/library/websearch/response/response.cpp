#include "response.h"
#include "factors.h"

#include <alice/library/analytics/common/names.h>

#include <google/protobuf/wrappers.pb.h>

#include <kernel/blender/factor_storage/serialization.h>

#include <contrib/libs/protobuf/src/google/protobuf/util/json_util.h>

namespace NAlice {

namespace {

template<typename TProtoMessage>
void FillProtoFromJson(const TString& json, TProtoMessage* result) {
    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = true;
    google::protobuf::util::JsonStringToMessage(json, result, options);
}

TString GetWebSearchResponsePartJsonString(const NSc::TValue& json, const TStringBuf path) {
    return json.TrySelect(path).ToJsonSafe();
}

void LogSearchReqId(TStringBuf reqid, const TString& requestActivationId, TRTLogger& logger) {
    LOG_INFO(logger) << "WebSearch reqid: " << reqid;

    auto* requestLogger = logger.RequestLogger();
    if (reqid.empty() || !requestLogger || requestActivationId.Empty()) {
        return;
    }

    auto ev = NRTLogEvents::SearchRequest();
    ev.SetReqId(TString{reqid});
    ev.SetActivationId(requestActivationId);
    requestLogger->LogEvent(ev);
}

} // anonymous namespace


TSearchResponse::TSearchResponse(
    const TString& json,
    const TString& requestActivationId,
    TRTLogger& logger,
    bool initDataSources
)
    : Data_{NSc::TValue::FromJsonThrow(json)}
    , Logger_{logger}
    , InitDataSources_{initDataSources}
{
    Init(requestActivationId);
}

TSearchResponse::TSearchResponse(
    TMaybe<NWebSearch::TTunnellerRawResponse>&& tunnellerRawResponsePart,
    TMaybe<NWebSearch::TStaticBlenderFactors>&& staticBlenderFactorsPart,
    TMaybe<NWebSearch::TReportGrouping>&& reportGroupingPart,
    TMaybe<NScenarios::TDataSource>&& docsDataSource,
    TMaybe<TBgFactors>&& bgFactorsPart,
    TRTLogger& logger
)
    : Logger_{logger}
    , BgFactors_{std::move(bgFactorsPart)}
    , TunnellerRawResponsePart_{std::move(tunnellerRawResponsePart)}
{
    if (staticBlenderFactorsPart.Defined()) {
        const auto& factors = staticBlenderFactorsPart->GetFactors();
        StaticBlenderFactors_.ConstructInPlace(factors.begin(), factors.end());
    }
    if (reportGroupingPart.Defined()) {
        ReportGrouping_.ConstructInPlace();
        auto& arr = ReportGrouping_->GetArrayMutable();
        for (const auto& gr : reportGroupingPart->GetRawGrouping()) {
            arr.push_back(NSc::TValue::FromJson(gr));
        }
    }
    if (docsDataSource.Defined()) {
        WebSearchDocs_ = docsDataSource->GetWebSearchDocs();
    }
}

const NSc::TValue& TSearchResponse::Report() const {
    return Data_["report"];
}

const TSearchResponse::TApplyBlenderFactors* TSearchResponse::StaticBlenderFactors() const {
    return StaticBlenderFactors_.Get();
}

const TSearchResponse::TBgFactors* TSearchResponse::BgFactors() const {
    return BgFactors_.Get();
}

const NSc::TValue& TSearchResponse::Data() const {
    return Data_;
}

const NScenarios::TWebSearchDocs& TSearchResponse::Docs() const {
    return WebSearchDocs_;
}

TMaybe<TString> TSearchResponse::GetTunnellerRawResponse() const {
    if (TunnellerRawResponsePart_.Defined() && TunnellerRawResponsePart_->HasRawResponse()) {
        return TunnellerRawResponsePart_->GetRawResponse().value();
    }
    if (Data().Has(NAnalyticsInfo::TUNNELLER_RAW_RESPONSE)) {
        return TString{Data()[NAnalyticsInfo::TUNNELLER_RAW_RESPONSE].GetString()};
    }
    return Nothing();
}

const NSc::TArray& TSearchResponse::GetReportGrouping() const {
    if (ReportGrouping_.Defined()) {
        return ReportGrouping_->GetArray();
    }
    return Report()["Grouping"].GetArray();
}

// DataSources
// Getters
const NScenarios::TWebSearchDocs& TSearchResponse::WebSearchDocs() const {
    if (InitDataSources_) {
        return WebSearchDocs_;
    }
    LOG_ERROR(Logger_) << "WebSearchDocs datasource has not been initialized";
    return Default<NScenarios::TWebSearchDocs>();
}

const NScenarios::TWebSearchDocsRight& TSearchResponse::WebSearchDocsRight() const {
    if (InitDataSources_) {
        return WebSearchDocsRight_;
    }
    LOG_ERROR(Logger_) << "WebSearchDocsRight datasource has not been initialized";
    return Default<NScenarios::TWebSearchDocsRight>();
}

const NScenarios::TWebSearchWizplaces& TSearchResponse::WebSearchWizplaces() const {
    if (InitDataSources_) {
        return WebSearchWizplaces_;
    }
    LOG_ERROR(Logger_) << "WebSearchWizplaces datasource has not been initialized";
    return Default<NScenarios::TWebSearchWizplaces>();
}

const NScenarios::TWebSearchSummarization& TSearchResponse::WebSearchSummarization() const {
    if (InitDataSources_) {
        return WebSearchSummarization_;
    }
    LOG_ERROR(Logger_) << "WebSearchSummarization datasource has not been initialized";
    return Default<NScenarios::TWebSearchSummarization>();
}

const NScenarios::TWebSearchWizard& TSearchResponse::WebSearchWizard() const {
    if (InitDataSources_) {
        return WebSearchWizard_;
    }
    LOG_ERROR(Logger_) << "WebSearchWizard datasource has not been initialized";
    return Default<NScenarios::TWebSearchWizard>();
}

const NScenarios::TWebSearchBanner& TSearchResponse::WebSearchBanner() const {
    if (InitDataSources_) {
        return WebSearchBanner_;
    }
    LOG_ERROR(Logger_) << "WebSearchBanner datasource has not been initialized";
    return Default<NScenarios::TWebSearchBanner>();
}

const NScenarios::TWebSearchRenderrer& TSearchResponse::WebSearchRenderrer() const {
    if (InitDataSources_) {
        return WebSearchRenderrer_;
    }
    LOG_ERROR(Logger_) << "WebSearchRenderrer datasource has not been initialized";
    return Default<NScenarios::TWebSearchRenderrer>();
}

// Initializers
void TSearchResponse::InitWebSearchDocs(const TSearchResponse& searchResponse, NScenarios::TWebSearchDocs* result) {
    FillProtoFromJson(GetWebSearchResponsePartJsonString(searchResponse.Data(), "tmpl_data/searchdata"), result);
}

void TSearchResponse::InitWebSearchDocsRight(const TSearchResponse& searchResponse, NScenarios::TWebSearchDocsRight* result) {
    FillProtoFromJson(GetWebSearchResponsePartJsonString(searchResponse.Data(), "tmpl_data/searchdata"), result);
}

void TSearchResponse::InitWebSearchWizplaces(const TSearchResponse& searchResponse, NScenarios::TWebSearchWizplaces* result) {
    FillProtoFromJson(GetWebSearchResponsePartJsonString(searchResponse.Data(), "tmpl_data/wizplaces"), result);
}

void TSearchResponse::InitWebSearchSummarization(const TSearchResponse& searchResponse, NScenarios::TWebSearchSummarization* result) {
    FillProtoFromJson(GetWebSearchResponsePartJsonString(searchResponse.Data(), "summarization"), result);
}

void TSearchResponse::InitWebSearchWizard(const TSearchResponse& searchResponse, NScenarios::TWebSearchWizard* result) {
    FillProtoFromJson(GetWebSearchResponsePartJsonString(searchResponse.Data(), "wizard"), result);
}

void TSearchResponse::InitWebSearchBanner(const TSearchResponse& searchResponse, NScenarios::TWebSearchBanner* result) {
    FillProtoFromJson(GetWebSearchResponsePartJsonString(searchResponse.Data(), "banner"), result);
}

void TSearchResponse::InitWebSearchRenderrer(const TSearchResponse& searchResponse, NScenarios::TWebSearchRenderrer* result) {
    result->SetResponse(GetWebSearchResponsePartJsonString(searchResponse.Data(), "renderrer"));
}


void TSearchResponse::Init(const TString& requestActivationId) {
    // Parse ApplyBlender factors.
    const NSc::TValue searcherProps = Data_["report"].Delete("SearcherProp");
    for (const NSc::TValue& searchProp : searcherProps.GetArray()) {
        const TStringBuf key = searchProp["Key"].GetString();
        const TStringBuf value = searchProp["Value"].GetString();

        if (key == TStringBuf("ApplyBlender.compressed_factors")) {
            StaticBlenderFactors_.ConstructInPlace();
            NBlender::NProtobufFactors::Decompress(
                StaticBlenderFactors_.Get(),
                /* dynamicFactors= */ nullptr,
                TString{value}
            );
            break;
        }
    }

    // Parse bgfactors.
    const NSearch::TFactorsMap relev = NSearch::ParseFactors(Data_["wizard"]["relev"].GetString());
    if (const TString* bgFactorsEncoded = relev.FindPtr("bgfactors")) {
        const TString bgFactorsDecoded = Base64Decode(*bgFactorsEncoded);
        BgFactors_.ConstructInPlace();
        Y_PROTOBUF_SUPPRESS_NODISCARD BgFactors_->ParseFromArray(bgFactorsDecoded.data(), bgFactorsDecoded.size());
    }

    LogSearchReqId(Data_["tmpl_data"]["reqdata"]["reqid"].GetString(), requestActivationId, Logger_);

    // This will be done without Init* functions right after tranfering scenario requests to apphost
    if (InitDataSources_) {
        InitWebSearchDocs(*this, &WebSearchDocs_);
        InitWebSearchDocsRight(*this, &WebSearchDocsRight_);
        InitWebSearchWizplaces(*this, &WebSearchWizplaces_);
        InitWebSearchSummarization(*this, &WebSearchSummarization_);
        InitWebSearchWizard(*this, &WebSearchWizard_);
        InitWebSearchBanner(*this, &WebSearchBanner_);
        InitWebSearchRenderrer(*this, &WebSearchRenderrer_);
    }
}


} // namespace NAlice
