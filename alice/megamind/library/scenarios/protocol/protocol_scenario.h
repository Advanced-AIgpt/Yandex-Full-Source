#pragma once

#include "helpers.h"

#include <alice/megamind/library/scenarios/defs/names.h>
#include <alice/megamind/library/scenarios/interface/protocol_scenario.h>

#include <alice/megamind/library/config/scenario_protos/config.pb.h>

#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/scenarios/request_meta.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/library/metrics/names.h>
#include <alice/library/network/common.h>
#include <alice/library/network/headers.h>
#include <alice/library/scenarios/utils/utils.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>

namespace NAlice::NImpl {

template <typename TProto>
[[nodiscard]] TErrorOr<TProto> ParseResponse(const NHttpFetcher::TResponse::TRef response, TStringBuf method) {
    if (!response || response->IsError()) {
        auto msg = TStringBuilder{} << "Failed to get response from the " << method << " request";
        if (response) {
            msg << ": " << response->GetErrorText();
            if (!response->Data.empty()) {
                msg << ", " << response->Data;
            }
            return ErrorFromResponseResult(response->Result, msg, response->Code);
        }
        return TError{TError::EType::Logic} << msg;
    }

    const auto* contentType = response->Headers.FindHeader(NNetwork::HEADER_CONTENT_TYPE);
    if (!contentType) {
        return TError{TError::EType::Http} << "No " << NNetwork::HEADER_CONTENT_TYPE << " in response";
    }
    if (contentType->Value() != NContentTypes::APPLICATION_PROTOBUF) {
        return TError{TError::EType::Http} << "Unsupported " << NNetwork::HEADER_CONTENT_TYPE
                                           << " value: " << contentType->Value();
    }

    return ParseProto<TProto>(response->Data, method);
}

TStringBuf GetResponseType(const NScenarios::TScenarioRunResponse& response);
TStringBuf GetResponseType(const NScenarios::TScenarioCommitResponse& response);
TStringBuf GetResponseType(const NScenarios::TScenarioApplyResponse& response);

TString GetBlurValue(const TString& uuid, const ui64 seed, const ui32 blurRatio);

} // namespace NAlice::NImpl

namespace NAlice {

namespace NTestSuiteProtocolScenario {

struct TTestCaseWriteMetrics;

} // namespace NTestSuiteProtocolScenario

namespace NTestSuiteFillRequestTests {

struct TTestCaseFillVinsRequestHintHeader;
struct TTestCaseFillRequestTickets;
struct TTestCaseFillRequestNoTickets;
struct TTestCaseFillRequestOAuthDisabled;
struct TTestCaseFillRequestOAuthEnabled;

} // namespace NTestSuiteFillRequestTests

static constexpr std::array<EDataSourceType, 5> WEB_SOURCES = {
    EDataSourceType::WEB_SEARCH_DOCS,
    EDataSourceType::WEB_SEARCH_DOCS_RIGHT,
    EDataSourceType::WEB_SEARCH_WIZPLACES,
    EDataSourceType::WEB_SEARCH_SUMMARIZATION,
    EDataSourceType::WEB_SEARCH_RENDERRER
};

class TConfigBasedProtocolScenario : public TProtocolScenario {
public:
    TConfigBasedProtocolScenario(const TScenarioConfig& config);

    bool IsEnabled(const IContext& ctx) const override;
    TVector<TString> GetAcceptedFrames() const override;
    bool AcceptsAnyUtterance() const override;
    bool AcceptsImageInput() const override;
    bool AcceptsMusicInput() const override;
    bool IsLanguageSupported(const ELanguage& language) const override;
    bool AlwaysRecieveAllParsedSemanticFrames() const override;

    const TScenarioConfig& GetConfig() const override {
        return Config;
    }

    bool DependsOnWebSearchResult() const override;
    TVector<EDataSourceType> GetDataSources() const override;
    TVector<EDataSourceType> GetRequiredDataSources() const override;

protected:
    template <typename TProto, typename TScenarioRequest>
    [[nodiscard]] TStatus FillRequest(const IContext& ctx, const TProto& proto, TScenarioRequest& request,
                                      bool enableOAuth) const  {
        if (auto error = FillHttpProxyRequest(ctx, proto, request, enableOAuth);
            error.Defined())
        {
            return TError{*error};
        }
        return Success();
    }

    void WriteSizeMetrics(const NScenarios::TScenarioRunResponse& response, NMetrics::ISensors& sensors) const;
    void WriteStateSize(ui64 sizeBytes, NMetrics::ISensors& sensors) const;

    template <typename TResponseProto>
    void WriteErrorOrProtoMetrics(const TErrorOr<TResponseProto>& errorOrProto, TStringBuf method, NMetrics::ISensors& sensors) const;

private:
    friend struct NAlice::NTestSuiteFillRequestTests::TTestCaseFillVinsRequestHintHeader;
    friend struct NAlice::NTestSuiteFillRequestTests::TTestCaseFillRequestTickets;
    friend struct NAlice::NTestSuiteFillRequestTests::TTestCaseFillRequestNoTickets;
    friend struct NAlice::NTestSuiteFillRequestTests::TTestCaseFillRequestOAuthDisabled;
    friend struct NAlice::NTestSuiteFillRequestTests::TTestCaseFillRequestOAuthEnabled;
    friend struct NAlice::NTestSuiteProtocolScenario::TTestCaseWriteMetrics;

    template <typename TResponseProto>
    TErrorOr<TResponseProto> ParseResponse(NHttpFetcher::TResponse::TRef response, TStringBuf method, NMetrics::ISensors& sensors) const;

    void WriteArgumentsSize(TStringBuf argumentsType, ui64 sizeBytes, NMetrics::ISensors& sensors) const;

protected:
    TScenarioConfig Config;

private:
    NSignal::TProtocolScenarioLabelsGenerator LabelsGenerator;
    TVector<TString> AcceptedFrames;
    THashSet<ELanguage> SupportedLanguages;
};

class TConfigBasedAppHostProxyProtocolScenario : public TConfigBasedProtocolScenario {
public:
    TConfigBasedAppHostProxyProtocolScenario(const TScenarioConfig& config);

    TStatus StartRun(const IContext& ctx,
                     const NScenarios::TScenarioRunRequest& request,
                     NMegamind::TItemProxyAdapter& itemProxyAdapter) const override;
    TErrorOr<NScenarios::TScenarioRunResponse> FinishRun(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const override;

    TErrorOr<NScenarios::TScenarioContinueResponse> FinishContinue(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const override;

    TStatus StartApply(const IContext& ctx,
                       const NScenarios::TScenarioApplyRequest& request,
                       NMegamind::TItemProxyAdapter& itemProxyAdapter) const override;
    TErrorOr<NScenarios::TScenarioApplyResponse> FinishApply(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const override;

    TStatus StartCommit(const IContext& ctx,
                        const NScenarios::TScenarioApplyRequest& request,
                        NMegamind::TItemProxyAdapter& itemProxyAdapter) const override;
    TErrorOr<NScenarios::TScenarioCommitResponse> FinishCommit(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const override;

    bool PassDataSourcesInContext(const IContext& ctx) const;

private:
    const TAppHostItemNames AppHostProxyItemNames;
    const TAppHostItemNames AppHostPureItemNames;
    const TString UseAppHostPureSceanrioFlag;
};

class TConfigBasedAppHostPureProtocolScenario : public TConfigBasedProtocolScenario {
public:
    TConfigBasedAppHostPureProtocolScenario(const TScenarioConfig& config);

    TStatus StartRun(const IContext& ctx,
                     const NScenarios::TScenarioRunRequest& request,
                     NMegamind::TItemProxyAdapter& itemProxyAdapter) const override;
    TErrorOr<NScenarios::TScenarioRunResponse> FinishRun(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const override;

    TErrorOr<NScenarios::TScenarioContinueResponse> FinishContinue(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const override;

    TStatus StartApply(const IContext& ctx,
                       const NScenarios::TScenarioApplyRequest& request,
                       NMegamind::TItemProxyAdapter& itemProxyAdapter) const override;
    TErrorOr<NScenarios::TScenarioApplyResponse> FinishApply(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const override;

    TStatus StartCommit(const IContext& ctx,
                        const NScenarios::TScenarioApplyRequest& request,
                        NMegamind::TItemProxyAdapter& itemProxyAdapter) const override;
    TErrorOr<NScenarios::TScenarioCommitResponse> FinishCommit(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const override;

private:
    const TAppHostItemNames AppHostPureItemNames;
};

} // namespace NAlice
