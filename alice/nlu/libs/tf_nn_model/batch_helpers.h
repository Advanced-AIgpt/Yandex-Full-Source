#pragma once

#include <contrib/libs/tf/tensorflow/core/framework/tensor.h>

#include <util/generic/vector.h>
#include <util/generic/algorithm.h>

namespace NVins {

template<typename T>
tensorflow::Tensor GetPaddedBatch(
    const TVector<TVector<T>>& data,
    size_t batchStart,
    size_t batchEnd,
    size_t paddingLength
);

template<typename R, typename I,  typename P>
TVector<R> ProcessInPaddedBatches(const TVector<I>& data, size_t batchSize, P processor) {
    TVector<R> result;
    result.reserve(data.size());
    for (size_t batchStart = 0; batchStart < data.size(); batchStart += batchSize) {
        auto batchResult = processor(data, batchStart, batchStart + batchSize);
        for (auto&& sampleResult : batchResult) {
            result.push_back(std::move(sampleResult));
        }
    }
    return result;
}

} // NVins
