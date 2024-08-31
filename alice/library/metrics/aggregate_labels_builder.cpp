#include "aggregate_labels_builder.h"

namespace NAlice {

TAggregateLabelsBuilder::TAggregateLabelsBuilder(const TString& prefix, const TLabelsNames& labelsNames)
    : Prefix(prefix)
    , AggregateLabelsNames(labelsNames) {
}

TAggregateLabelsBuilder::TAggregateLabelsBuilder(const TString& prefix, TLabelsNames&& labelsNames)
    : Prefix(prefix)
    , AggregateLabelsNames(std::move(labelsNames)) {
}

TVector<NMonitoring::TLabels> TAggregateLabelsBuilder::Build(const NMonitoring::TLabels& originalSensorLabels) const {
    TVector<NMonitoring::TLabels> result(Reserve(AggregateLabelsNames.size()));
    for (const auto& aggregateLabelName : AggregateLabelsNames) {
        NMonitoring::TLabels subResult(Reserve(originalSensorLabels.size()));
        for (const auto& label : originalSensorLabels) {
            if (aggregateLabelName.contains(label.Name())) {
                subResult.Add(label.Name(), Prefix + label.Name());
            } else {
                subResult.Add(label.Name(), label.Value());
            }
        }
        result.push_back(subResult);
    }

    return result;
}

} // namespace NAlice
