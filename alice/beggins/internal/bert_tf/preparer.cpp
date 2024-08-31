#include "preparer.h"

#include <util/generic/ptr.h>
#include <util/generic/xrange.h>
#include <util/stream/file.h>

namespace NAlice::NBeggins::NBertTfApplier {

std::unique_ptr<TBertDict> TBertDict::FromFile(const TString& vocabFilePath) {
    auto dict = std::make_unique<THashMap<TUtf32String, size_t>>();
    TFileInput vocabFile(vocabFilePath);
    TString token;
    for (size_t tokenId = 0; vocabFile.ReadLine(token); tokenId++) {
        dict->insert({TUtf32String::FromUtf8(token), tokenId});
    }
    return std::make_unique<TBertDict>(std::move(dict));
}

TBertDict::TBertDict(std::unique_ptr<THashMap<TUtf32String, size_t>> dict)
    : TokenIds(std::move(dict)) {
    Y_ENSURE(TokenIds);
    UnkId = TokenIds->at(UNK);
}

TVector<size_t> TBertDict::TokensToIds(const TVector<TUtf32String>& tokens) const {
    TVector<size_t> result(Reserve(tokens.size()));
    for (const auto& token : tokens) {
        const auto tokenIdPtr = TokenIds->FindPtr(token);
        result.push_back(tokenIdPtr ? *tokenIdPtr : UnkId);
    }
    return result;
}

TBatchPreparer::TBatchPreparer(std::unique_ptr<TBertDict> vocab, size_t sequenceLength)
    : Dict(std::move(vocab))
    , SequenceLength(sequenceLength) {
    Y_ENSURE(Dict);
}

TBatchPreparer::TPreparedTensors TBatchPreparer::Prepare(const TVector<TVector<TUtf32String>>& tokens) const {
    const auto nSamples = tokens.size();
    const tensorflow::TensorShape shape = {static_cast<i64>(nSamples), static_cast<i64>(SequenceLength)};
    TPreparedTensors result{.InputIds = tensorflow::Tensor(tensorflow::DT_INT32, shape),
                            .SegmentIds = tensorflow::Tensor(tensorflow::DT_INT32, shape),
                            .InputMask = tensorflow::Tensor(tensorflow::DT_INT32, shape)};

    auto inputIdsMap = result.InputIds.template tensor<int, 2>();
    auto inputMaskMap = result.InputMask.template tensor<int, 2>();
    auto segmentIdsMap = result.SegmentIds.template tensor<int, 2>();
    inputIdsMap.setZero();
    inputMaskMap.setZero();
    segmentIdsMap.setZero();

    for (auto sampleIdx : xrange(nSamples)) {
        const auto tokenIds = Dict->TokensToIds(tokens[sampleIdx]);
        const auto nTokens = tokenIds.size();
        for (const auto tokenIdx : xrange(nTokens)) {
            inputIdsMap(sampleIdx, tokenIdx) = tokenIds[tokenIdx];
            inputMaskMap(sampleIdx, tokenIdx) = 1;
        }
    }
    return result;
}
} // namespace NAlice::NBeggins::NBertTfApplier
