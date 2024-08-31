#include "hardcoded_response_fast_data.h"

#include <alice/megamind/protos/scenarios/request.pb.h>

#include <library/cpp/iterator/mapped.h>

#include <util/generic/yexception.h>
#include <util/string/join.h>

namespace NAlice::NHollywood {

// THardcodedResponse
THardcodedResponse::THardcodedResponse(const THardcodedResponseFastDataProto::TRawHardcodedResponse& rawHardcodedResponse)
    : Name_(rawHardcodedResponse.GetName())
    , Responses_(rawHardcodedResponse.GetResponses().begin(), rawHardcodedResponse.GetResponses().end())
    , ChildResponses_(rawHardcodedResponse.GetChildResponses().begin(), rawHardcodedResponse.GetChildResponses().end())
    , Links_(rawHardcodedResponse.GetLinks().begin(), rawHardcodedResponse.GetLinks().end())
    // , Regexp_(TRegExMatch(CombineRegexps(rawHardcodedResponse.GetRegexps()), REG_UTF8 | REG_NOSUB | REG_EXTENDED))
{
    if (!rawHardcodedResponse.GetAppIdRegexp().empty()) {
        AppIdRegexp_ =
            TRegExMatch(NImpl::AddBeginEndToRegexp(NImpl::EncloseRegexp(rawHardcodedResponse.GetAppIdRegexp())),
                        REG_UTF8 | REG_NOSUB | REG_EXTENDED);
    }

    auto range = MakeMappedRange(
        rawHardcodedResponse.GetRegexps(),
        [](const TString& regexp) {
            return NImpl::EncloseRegexp(regexp);
        }
    );

    Regexp_ = TRegExMatch(NImpl::AddBeginEndToRegexp(NImpl::EncloseRegexp(JoinSeq("|", range))),
                          REG_UTF8 | REG_NOSUB | REG_EXTENDED);

    for (const auto promoType : rawHardcodedResponse.GetEnableForPromoTypes()) {
        PromoTypes_.insert(static_cast<NClient::EPromoType>(promoType));
    }
}

const TString& THardcodedResponse::Name() const {
    return Name_;
}

bool THardcodedResponse::Match(const TString& utterance, const TString& appId, const NClient::EPromoType promoType) const {
    if (AppIdRegexp_.Defined() && !AppIdRegexp_->Match(appId.data())) {
        return false;
    }
    if (!PromoTypes_.empty() && !PromoTypes_.contains(promoType)) {
        return false;
    }
    if (Regexp_.Match(utterance.data())) {
        return true;
    }
    return false;
}

const THardcodedResponseFastDataProto::TResponse& THardcodedResponse::GetResponse(IRng& rng) const {
    return Responses_.at(rng.RandomInteger(0, Responses_.size()));
}

bool THardcodedResponse::HasChildResponse() const {
    return !ChildResponses_.empty();
}

const THardcodedResponseFastDataProto::TResponse& THardcodedResponse::GetChildResponse(IRng& rng) const {
    return ChildResponses_.at(rng.RandomInteger(0, ChildResponses_.size()));
}

const THardcodedResponseFastDataProto::TLink* THardcodedResponse::GetLinkPtr(IRng& rng) const {
    if (Links_.empty()) {
        return nullptr;
    }
    return &Links_.at(rng.RandomInteger(0, Links_.size()));
}

// TGranetHardcodedResponse
TGranetHardcodedResponse::TGranetHardcodedResponse(
    const THardcodedResponseFastDataProto::TGranetHardcodedResponseProto& proto)
    : Proto{proto}
    , ApplicabilityWrapper{proto.GetApplicabilityInfo()}
{
    if (const auto& psn = Proto.GetProductScenarioName(); !psn.Empty()) {
        ProductScenarioName = psn;
    }
}

const TString& TGranetHardcodedResponse::Name() const {
    return Proto.GetName();
}

bool TGranetHardcodedResponse::HasFallback() const {
    return Proto.HasFallbackResponse();
}

const TString& TGranetHardcodedResponse::GetIntent() const {
    return Proto.GetIntent();
}
const TMaybe<TString>& TGranetHardcodedResponse::GetProductScenarioName() const {
    return ProductScenarioName;
}

const THardcodedResponseFastDataProto::TResponse& TGranetHardcodedResponse::GetResponse(IRng& rng) const {
    Y_ENSURE(!Proto.GetResponses().empty());
    return Proto.GetResponses().at(rng.RandomInteger(0, Proto.GetResponses().size()));
}
const THardcodedResponseFastDataProto::TResponse& TGranetHardcodedResponse::GetFallbackResponse(IRng& rng) const {
    const auto& fallbackResponses = Proto.GetFallbackResponse().GetResponses();
    Y_ENSURE(!fallbackResponses.empty());
    return fallbackResponses.at(rng.RandomInteger(0, fallbackResponses.size()));
}

bool TGranetHardcodedResponse::HasLink() const {
    return Proto.HasLink();
}

const THardcodedResponseFastDataProto::TLink& TGranetHardcodedResponse::GetLink() const {
    return Proto.GetLink();
}
bool TGranetHardcodedResponse::HasFallbackPush() const {
    return Proto.GetFallbackResponse().HasPushDirective();
}

const THardcodedResponseFastDataProto::TPushDirective& TGranetHardcodedResponse::GetFallbackPush() const {
    return Proto.GetFallbackResponse().GetPushDirective();
}

bool TGranetHardcodedResponse::IsApplicable(const TScenarioRunRequestWrapper& request, TRTLogger& logger) const {
    return ApplicabilityWrapper.IsApplicable(request, logger);
}

// THardcodedResponseFastData
THardcodedResponseFastData::THardcodedResponseFastData(const THardcodedResponseFastDataProto& proto) {
    for (const auto& response : proto.GetResponses()) {
        HardcodedResponses_.emplace_back(response);
    }
    for (const auto& response : proto.GetGranetResponses()) {
        GranetHardcodedResponses_[response.GetName()].emplace_back(response);
    }
}

const THardcodedResponse* THardcodedResponseFastData::FindPtr(const TString& utterance, const TString& appId, const NClient::EPromoType promoType) const {
    return FindIfPtr(HardcodedResponses_, [&utterance, &appId, promoType](const THardcodedResponse& response) {
        return response.Match(utterance, appId, promoType);
    });
}

const TGranetHardcodedResponse*
THardcodedResponseFastData::FindPtr(const TString& name, const TScenarioRunRequestWrapper& request, TRTLogger& logger) const {
    if (const auto* responses = GranetHardcodedResponses_.FindPtr(name)) {
        for (const auto& response : *responses) {
            if (response.IsApplicable(request, logger)) {
                return &response;
            }
        }
        for (const auto& response : *responses) {
            if (response.HasFallback()) {
                return &response;
            }
        }
    }
    return nullptr;
}

} // namespace NAlice::NHollywood
