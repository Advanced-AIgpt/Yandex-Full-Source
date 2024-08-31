#pragma once

#include "applicability_wrapper.h"

#include <alice/hollywood/library/scenarios/hardcoded_response/proto/hardcoded_response.pb.h>

#include <alice/hollywood/library/fast_data/fast_data.h>
#include <alice/hollywood/library/request/fwd.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/library/client/protos/promo_type.pb.h>
#include <alice/library/util/rng.h>

#include <library/cpp/regex/pcre/regexp.h>

#include <util/generic/hash_set.h>
#include <util/generic/vector.h>

namespace NAlice::NHollywood {

class THardcodedResponse {
public:
    THardcodedResponse(const THardcodedResponseFastDataProto::TRawHardcodedResponse& rawHardcodedResponse);

    const TString& Name() const;

    bool Match(const TString& utterance, const TString& appId, const NClient::EPromoType promoType) const;

    const THardcodedResponseFastDataProto::TResponse& GetResponse(IRng& rng) const;

    bool HasChildResponse() const;

    const THardcodedResponseFastDataProto::TResponse& GetChildResponse(IRng& rng) const;

    const THardcodedResponseFastDataProto::TLink* GetLinkPtr(IRng& rng) const;

private:
    TString Name_;
    TVector<THardcodedResponseFastDataProto::TResponse> Responses_;
    TVector<THardcodedResponseFastDataProto::TResponse> ChildResponses_;
    TVector<THardcodedResponseFastDataProto::TLink> Links_;

    TMaybe<TRegExMatch> AppIdRegexp_;
    TRegExMatch Regexp_;
    THashSet<NClient::EPromoType> PromoTypes_;
};

class TGranetHardcodedResponse {
public:
    TGranetHardcodedResponse(const THardcodedResponseFastDataProto::TGranetHardcodedResponseProto& proto);

    const TString& Name() const;
    const TString& GetIntent() const;
    const TMaybe<TString>& GetProductScenarioName() const;

    bool HasFallback() const;

    const THardcodedResponseFastDataProto::TResponse& GetResponse(IRng& rng) const;
    const THardcodedResponseFastDataProto::TResponse& GetFallbackResponse(IRng& rng) const;

    bool HasLink() const;
    const THardcodedResponseFastDataProto::TLink& GetLink() const;

    bool HasFallbackPush() const;
    const THardcodedResponseFastDataProto::TPushDirective& GetFallbackPush() const;

    bool IsApplicable(const TScenarioRunRequestWrapper& request, TRTLogger& logger) const;

private:
    THardcodedResponseFastDataProto::TGranetHardcodedResponseProto Proto;
    TRequestApplicabilityWrapper ApplicabilityWrapper;
    TMaybe<TString> ProductScenarioName;
};

class THardcodedResponseFastData : public IFastData {
public:
    THardcodedResponseFastData(const THardcodedResponseFastDataProto& proto);

    const THardcodedResponse* FindPtr(const TString& utterance, const TString& appId, const NClient::EPromoType promoType) const;

    const TGranetHardcodedResponse* FindPtr(const TString& name, const TScenarioRunRequestWrapper& request,
                                            TRTLogger& logger) const;

private:
    TVector<THardcodedResponse> HardcodedResponses_;
    THashMap<TString, TVector<TGranetHardcodedResponse>> GranetHardcodedResponses_;
};

} // namespace NAlice::NHollywood
