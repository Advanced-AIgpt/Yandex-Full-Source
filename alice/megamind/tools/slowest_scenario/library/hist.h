#pragma once

#include <library/cpp/yson/node/node_io.h>

namespace NAlice::NSlowestScenario {

using TIntervals = TVector<double>;

struct THist {
    THist(const TIntervals& intervals)
        : Buckets(intervals.size(), 0)
        , Count{0}
        , Intervals{intervals}
    {
    }

    THist(const THist& rhs) = default;
    THist(THist&& rhs) = default;

    void Add(ui64 diff);

    NYT::TNode ToNode() const;

    static THist FromNode(const NYT::TNode& node, const TIntervals& intervals);

    void Merge(const THist& rhs);

    double ComputePercentile(double perc) const;

    template <typename TList>
    TVector<double> ComputePercentiles(const TList& percs) const {
        // XXX: Suboptimal, can be done in one array pass.
        TVector<double> res(Reserve(std::distance(percs.begin(), percs.end())));
        for (auto perc : percs) {
            res.push_back(ComputePercentile(perc));
        }
        return res;
    }

    TVector<size_t> Buckets;
    size_t Count;
    const TIntervals& Intervals;
};

} // namespace NAlice::NSlowestScenario
