#pragma once

#include <alice/library/experiments/experiments.h>
#include <alice/megamind/library/classifiers/formulas/protos/formulas_description.pb.h>
#include <alice/megamind/library/experiments/flags.h>

#include <util/generic/fwd.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>


namespace NAlice::NMegamind {

/* For example, TExperimentFlagsMap could be a
    - THashMap<String, __anything__>
    - google::protobuf::Map<TProtoStringType, __anything__>
    - ...
*/

template <typename TExperimentFlagsMap>
TVector<TStringBuf> FindExperimentsWithSubtype(const TExperimentFlagsMap& flags, const TStringBuf experiment, const TStringBuf subtype, const TStringBuf ending = {}) {
    TVector<TStringBuf> ans;
    for (const auto& [key, val] : flags) {
        TStringBuf subtypeSuffix;
        if (!TStringBuf{key}.AfterPrefix(experiment, subtypeSuffix)) {
            continue;
        }
        TStringBuf endingSuffix;
        if (!subtypeSuffix.AfterPrefix(subtype, endingSuffix)) {
            continue;
        }
        TStringBuf value;
        if (!endingSuffix.AfterPrefix(ending, value)) {
            continue;
        }
        ans.push_back(value);
    }
    return ans;
}

template <typename TExperimentFlagsMap>
TMaybe<TStringBuf> GetMMFormulaExperimentForSpecificClient(const TExperimentFlagsMap& flags, const EClientType clientType) {
    const TStringBuf client = EClientType_Name(clientType);

    const TVector<TStringBuf>& suffixes = FindExperimentsWithSubtype(flags, EXP_PREFIX_MM_FORMULA_FOR_SPECIFIC_CLIENT, client, "=");
    if (!suffixes.empty()) {
        return suffixes[0];
    }
    
    return GetExperimentValueWithPrefix(flags, EXP_PREFIX_MM_FORMULA);
}

template <typename TExperimentFlagsMap>
TMaybe<TStringBuf> GetPreclassificationConfidentFramesExperiment(const TExperimentFlagsMap& flags, TStringBuf scenario) {
    const TVector<TStringBuf>& suffixes = FindExperimentsWithSubtype(flags, EXP_ADD_PRECLASSIFIER_CONFIDENT_FRAME_PREFIX, scenario, "=");
    if (!suffixes.empty()) {
        return suffixes[0];
    }
    
    return Nothing();
}

} // namespace NAlice::NMegamind
