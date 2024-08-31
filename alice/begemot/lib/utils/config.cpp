#include "config.h"

namespace NAlice {

bool IsEnabledByExperiments(const ::google::protobuf::RepeatedPtrField<TProtoStringType>& expressions,
    const THashSet<TString>& experiments)
{
    for (const TString& expression : expressions) {
        TStringBuf experiment = expression;
        const bool isNegative = experiment.SkipPrefix("!");
        if (isNegative == experiments.contains(experiment)) {
            return false;
        }
    }
    return true;
}

} // namespace NAlice
