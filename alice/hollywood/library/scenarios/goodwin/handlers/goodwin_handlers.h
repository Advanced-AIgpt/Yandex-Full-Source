#pragma once

#include "search_doc.h"

#include <alice/hollywood/library/frame_filler/lib/frame_filler_scenario_handlers.h>
#include <alice/hollywood/library/http_requester/http_requester.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NAlice {
namespace NFrameFiller {

TFrameFillerScenarioResponse ToScenarioResponse(const TString& string);

class TGoodwinScenarioRunHandler : public IFrameFillerScenarioRunHandler {
public:
    TGoodwinScenarioRunHandler(
        TSimpleSharedPtr<NHollywood::IHttpRequester> urlRequester,
        TAcceptDocPredicate acceptDoc
    );

    TFrameFillerScenarioResponse Do(
        const NHollywood::TScenarioRunRequestWrapper& request,
        TRTLogger& logger
    ) const override;

private:
    TFrameFillerScenarioResponse DoImpl(
        const NHollywood::TScenarioRunRequestWrapper& request,
        TRTLogger& logger
    ) const;
    TFrameFillerScenarioResponse ProcessRequestUrlDirective(
            const NScenarios::TCallbackDirective& directive,
            const NScenarios::TInput& input,
            const NScenarios::TScenarioBaseRequest& baseRequest,
            const NScenarios::TDataSource* blackbox,
            TRTLogger& logger
        ) const;
    TFrameFillerScenarioResponse ProcessGoodwinUrl(
            const TString& urlTemplate,
            const ::google::protobuf::Value* payload,
            const NJson::TJsonValue& fetchedData,
            const NScenarios::TInput& input,
            const NScenarios::TScenarioBaseRequest& baseRequest,
            const NScenarios::TDataSource* blackbox
        ) const;
    TMaybe<NJson::TJsonValue> GetGoodwinDoc(const NJson::TJsonValue& renderrerResponse) const;
    TFrameFillerScenarioResponse ProcessGoodwinResponse(
        const NHollywood::TScenarioRunRequestWrapper& request,
        TRTLogger& logger
    ) const;

private:
    TSimpleSharedPtr<NHollywood::IHttpRequester> UrlRequester;
    TAcceptDocPredicate AcceptDoc;
};


class TGoodwinScenarioCommitHandler : public IFrameFillerScenarioCommitHandler {
public:
    explicit TGoodwinScenarioCommitHandler(TSimpleSharedPtr<NHollywood::IHttpRequester> urlRequester);

    NScenarios::TScenarioCommitResponse Do(
        const NHollywood::TScenarioApplyRequestWrapper& request,
        TRTLogger& logger
    ) const override;

private:
    NScenarios::TScenarioCommitResponse DoImpl(
        const NHollywood::TScenarioApplyRequestWrapper& request,
        TRTLogger& logger
    ) const;

private:
    TSimpleSharedPtr<NHollywood::IHttpRequester> UrlRequester;
};


class TGoodwinScenarioApplyHandler : public IFrameFillerScenarioApplyHandler {
public:
    NScenarios::TScenarioApplyResponse Do(
        const NHollywood::TScenarioApplyRequestWrapper& request,
        TRTLogger& logger
    ) const override;
};


static constexpr char REQUEST_URL_CALLBACK[] = "request_url";
static constexpr char REQUEST_URL_WITH_SIDE_EFFECT_URLS_CALLBACK[] = "request_url_with_side_effect_urls";

} // namespace NFrameFiller
} // namespace NAlice
