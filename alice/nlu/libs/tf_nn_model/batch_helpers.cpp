#include "batch_helpers.h"
#include "tensor_helpers.h"
#include <util/generic/yexception.h>

using namespace NVins;
using namespace tensorflow;

namespace NVins {

namespace {
    const i64 UNDEFINED_TOKEN_DATA_SIZE = -1;
} // namespace anonymous

template<>
Tensor GetPaddedBatch<TVector<float>>(
    const TVector<TVector<TVector<float>>>& data,
    size_t batchStart,
    size_t batchEnd,
    size_t paddingLength
) {
    batchEnd = Min(data.size(), batchEnd);
    const size_t batchSize = batchEnd - batchStart;

    i64 tokenDataSize = UNDEFINED_TOKEN_DATA_SIZE;
    Y_ENSURE(!data.empty(), "Batch is empty.");
    for (const auto& sample : data) {
        for (const auto& token : sample) {
            Y_ENSURE(!token.empty());
            if (tokenDataSize == UNDEFINED_TOKEN_DATA_SIZE) {
                tokenDataSize = token.ysize();
            }
            Y_ENSURE(tokenDataSize == token.ysize(), "Not all tokens have same data size.");
        }
    }
    Y_ENSURE(tokenDataSize != UNDEFINED_TOKEN_DATA_SIZE, "Unable to determine data size per token.");
    Tensor paddedBatch = MakeZeroTensor<float>({static_cast<i64>(batchSize), static_cast<i64>(paddingLength), tokenDataSize});
    auto&& paddedBatchData = paddedBatch.tensor<float, 3>();

    for (size_t sampleIdx = 0; sampleIdx < batchSize; ++sampleIdx) {
        const auto& sample = data[batchStart + sampleIdx];
        size_t dim2Size = paddedBatchData.dimension(2);
        for (size_t dim2 = 0; dim2 < dim2Size; ++dim2) {
            for (size_t dim1 = 0; dim1 < paddingLength && dim1 < sample.size(); ++dim1) {
                paddedBatchData(
                    static_cast<i64>(sampleIdx),
                    static_cast<i64>(dim1),
                    static_cast<i64>(dim2)
                ) = sample[dim1][dim2];
            }
        }
    }

    return paddedBatch;
}

template<>
Tensor GetPaddedBatch<i32>(
    const TVector<TVector<i32>>& data,
    size_t batchStart,
    size_t batchEnd,
    size_t paddingLength
) {
    batchEnd = Min(data.size(), batchEnd);
    size_t batchSize = batchEnd - batchStart;

    Tensor paddedBatch = MakeZeroTensor<i32>({
        static_cast<i64>(batchSize),
        static_cast<i64>(paddingLength)
    });
    auto&& paddedBatchData = paddedBatch.tensor<i32, 2>();

    for (size_t sampleIdx = 0; sampleIdx < batchSize; ++sampleIdx) {
        const auto& sample = data[batchStart + sampleIdx];
        for (size_t dim1 = 0; dim1 < paddingLength && dim1 < sample.size(); ++dim1) {
            paddedBatchData(
                static_cast<i64>(sampleIdx),
                static_cast<i64>(dim1)
            ) = sample[dim1];
        }
    }

    return paddedBatch;
}

} // NVins
