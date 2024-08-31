#pragma once

#include "literals.h"

#include <tensorflow/core/framework/tensor.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>

namespace NAlice::NBeggins::NBertTfApplier {

class TBertDict {
public:
    static std::unique_ptr<TBertDict> FromFile(const TString& vocabFilePath);
    TVector<size_t> TokensToIds(const TVector<TUtf32String>& tokens) const;
    TBertDict(std::unique_ptr<THashMap<TUtf32String, size_t>> vocab);

private:
    std::unique_ptr<THashMap<TUtf32String, size_t>> TokenIds;
    size_t UnkId;
};

class TBatchPreparer {
public:
    struct TPreparedTensors {
        tensorflow::Tensor InputIds;
        tensorflow::Tensor SegmentIds;
        tensorflow::Tensor InputMask;
    };

    TBatchPreparer(std::unique_ptr<TBertDict> vocab, size_t sequenceLength);
    TPreparedTensors Prepare(const TVector<TVector<TUtf32String>>& tokens) const;

private:
    std::unique_ptr<TBertDict> Dict;
    size_t SequenceLength;
};
} // namespace NAlice::NBeggins::NBertTfApplier
