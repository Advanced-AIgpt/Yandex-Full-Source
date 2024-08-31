#include "applicability_wrapper.h"

#include <alice/hollywood/library/request/request.h>

namespace NAlice::NHollywood {

namespace {

bool CheckFeatureSupport(const TStringBuf feature, const TScenarioRunRequestWrapper& request, TRTLogger& logger) {
    const auto& interfaces = request.BaseRequestProto().GetInterfaces();
    const NProtoBuf::Descriptor* descriptor = interfaces.GetDescriptor();
    Y_ASSERT(descriptor != nullptr);
    const NProtoBuf::FieldDescriptor* feature_field = descriptor->FindFieldByName(TString{feature});

    if (feature_field == nullptr || feature_field->type() != NProtoBuf::FieldDescriptor::TYPE_BOOL) {
        LOG_ERROR(logger) << "Requested feature is unknown: " << feature;
        return false;
    }

    const NProtoBuf::Reflection* reflection = interfaces.GetReflection();
    Y_ASSERT(reflection != nullptr);
    if (!reflection->GetBool(interfaces, feature_field)) {
        LOG_INFO(logger) << "Feature is unsupported: " << feature;
        return false;
    }

    LOG_INFO(logger) << "Feature is supported: " << feature;
    return true;
}
} // namespace

namespace NImpl {

TString EncloseRegexp(const TStringBuf regexp) {
    return TString::Join("(?:", regexp, ")");
}

TString AddBeginEndToRegexp(const TStringBuf regexp) {
    return TString::Join("^", regexp, "$");
}
} // namespace NImpl

TRequestApplicabilityWrapper::TRequestApplicabilityWrapper(
    const THardcodedResponseFastDataProto::TApplicabilityInfo& proto)
    : Proto{proto}
{
    if (proto.HasAppIdRegexp()) {
        AppIdRegexp_.ConstructInPlace(NImpl::AddBeginEndToRegexp(NImpl::EncloseRegexp(Proto.GetAppIdRegexp())),
                                      REG_UTF8 | REG_NOSUB | REG_EXTENDED);
    }
}

bool TRequestApplicabilityWrapper::IsApplicable(const TScenarioRunRequestWrapper& request, TRTLogger& logger) const {
    const bool isChild =
        request.Proto().GetBaseRequest().GetUserClassification().GetAge() == NScenarios::TUserClassification::Child;
    const auto promoType = request.Proto().GetBaseRequest().GetOptions().GetPromoType();
    const auto& appId = request.Proto().GetBaseRequest().GetClientInfo().GetAppId();

    if (Proto.GetDisabledForChildren() && isChild) {
        LOG_INFO(logger) << "Disabled by for children";
        return false;
    }

    if (const auto& promoTypes = Proto.GetEnableForPromoTypes(); !promoTypes.empty()) {
        const bool haveEnabled = AnyOf(promoTypes, [&](const auto requiredPromoType) {
            return promoType == static_cast<NClient::EPromoType>(requiredPromoType);
        });
        if (!haveEnabled) {
            LOG_INFO(logger) << "Disabled by promo type";
            return false;
        }
    }

    if (AppIdRegexp_.Defined() && !AppIdRegexp_->Match(appId.data())) {
        LOG_INFO(logger) << "Disabled by AppIdRegexp";
        return false;
    }

    if (Proto.HasSupportedFeature() &&
        !CheckFeatureSupport(Proto.GetSupportedFeature(), request, logger)) {
        LOG_INFO(logger) << "Disabled by supported feature";
        return false;
    }

    if (Proto.HasExperiment() && !request.HasExpFlag(Proto.GetExperiment())) {
        LOG_INFO(logger) << "Disabled by experiment";
        return false;
    }

    LOG_INFO(logger) << "Enabled";

    return true;
}

} // namespace NAlice::NHollywood
