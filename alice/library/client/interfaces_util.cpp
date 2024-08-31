#include "interfaces_util.h"

namespace NAlice {

bool CheckFeatureSupport(const NScenarios::TInterfaces& interfaces, const TStringBuf feature, TRTLogger& logger) {
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
        LOG_DEBUG(logger) << "Feature is unsupported: " << feature;
        return false;
    }

    LOG_DEBUG(logger) << "Feature is supported: " << feature;
    return true;
}

} // namespace NAlice
