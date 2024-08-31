#include "hist.h"

#include <library/cpp/iterator/enumerate.h>

namespace NAlice::NSlowestScenario {

using namespace NYT;

// THist -----------------------------------------------------------------------
void THist::Add(ui64 diff) {
    ++Count;
    auto iter = std::upper_bound(Intervals.begin(), Intervals.end(), diff);
    if (iter != Intervals.end()) {
        auto index = std::distance(Intervals.begin(), iter);
        ++Buckets[index];
    }
}

TNode THist::ToNode() const {
    TNode node;
    auto nodeHist = TNode::CreateList();
    for (auto value : Buckets) {
        nodeHist.Add(value);
    }
    node["diff"] = std::move(nodeHist);
    node["count"] = Count;
    return node;
}

THist THist::FromNode(const TNode& node, const TIntervals& intervals) {
    THist hist{intervals};
    hist.Count = node["count"].AsUint64();
    for (auto [index, value] : Enumerate(node["diff"].AsList())) {
        hist.Buckets[index] = value.AsUint64();
    }
    return hist;
}

void THist::Merge(const THist& rhs) {
    Count += rhs.Count;
    for (auto [idx, value] : Enumerate(rhs.Buckets)) {
        Buckets[idx] += value;
    }
}

double THist::ComputePercentile(double perc) const {
    double expectedCnt = Count * perc;
    size_t bucket = 0;
    size_t cnt = 0;
    while (cnt < expectedCnt && bucket < Intervals.size()) {
        ++bucket;
        cnt += Buckets[bucket];
    }
    size_t prevCnt = cnt - Buckets[bucket];
    double fromTime = Intervals[bucket - 1];
    double toTime = Intervals[bucket];
    double res = fromTime + (expectedCnt - prevCnt) / Buckets[bucket] * (toTime - fromTime);
    return res;
}

} // namespace NAlice::NSlowestScenario
