#pragma once

#include <library/cpp/monlib/metrics/labels.h>

#include <util/generic/hash_set.h>

namespace NAlice {

using TLabelsNames = TVector<THashSet<TString>>;

class TAggregateLabelsBuilder final {
public:
    TAggregateLabelsBuilder() = default;
    TAggregateLabelsBuilder(const TString& prefix, const TLabelsNames& labelsNames);
    TAggregateLabelsBuilder(const TString& prefix, TLabelsNames&& labelsNames);

    TVector<NMonitoring::TLabels> Build(const NMonitoring::TLabels& originalSensorLabels) const;

private:
    TString Prefix;
    TLabelsNames AggregateLabelsNames;
};

} // namespace NAlice
