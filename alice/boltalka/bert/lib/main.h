#pragma once
#include <util/string/split.h>

#include <util/generic/algorithm.h>
#include <util/generic/array_ref.h>
#include <util/generic/reserve.h>
#include <util/string/join.h>
#include <util/system/info.h>

#include <library/cpp/float16/float16.h>
#include <library/cpp/getopt/last_getopt.h>

#include <dict/mt/libs/nn/ynmt_backend/gpu/backend.h>
#include <dict/mt/libs/nn/ynmt_backend/cpu/backend.h>

#include <dict/mt/libs/nn/ynmt/config_helper/model_reader.h>

#include <dict/mt/libs/nn/ynmt/extra/encoder_head.h>

#include <kernel/bert/bert_wrapper.h>
#include <kernel/bert/tokenizer.h>

#include <type_traits>

using NBertApplier::TBertWrapper;
using NDict::NMT::NYNMT::TRegressionHead;

TVector<TUtf32String> Tokenize(const TString& s, THolder<NBertApplier::TBertSegmenter>& segmenter) {
    // split input request into tokens
    TUtf32String w32Sentence = TUtf32String::FromUtf8(s);
    return segmenter->Split(w32Sentence);
}

TVector<TVector<int>> PreprocessSamples(
    const TVector<TString>& samples, size_t maxInputLength, const TString& startTrieFilename, const TString& contTrieFilename, const TString& vocabFilename) {
    TVector<TVector<int>> result(Reserve(samples.size()));

    THolder<NBertApplier::TBertSegmenter> segmenter(new NBertApplier::TBertSegmenter(startTrieFilename, contTrieFilename));
    THolder<NBertApplier::TBertIdConverter> converter(new NBertApplier::TBertIdConverter(vocabFilename));

    for (auto& sample : samples) {
        TVector<TUtf32String> tokens = Tokenize(sample, segmenter);
        size_t num_tokens = Min(tokens.size(), maxInputLength - 2);
        tokens.resize(num_tokens);
        auto tokenIds = converter->Convert(tokens);
        result.emplace_back(tokenIds.begin(), tokenIds.end());
    }
    return result;
}

template <typename TFloatType>
class IBertApplier {
public:
    virtual TVector<TFloatType> ProcessBatch(const TConstArrayRef<TVector<int>>& batch) = 0;
    virtual ~IBertApplier() = default;
};

NDict::NMT::NYNMT::TBackendPtr CreateBackend(bool useCpu, int deviceIndex, TMaybe<size_t> numThreads) {
    if (useCpu) {
        return NDict::NMT::NYNMT::CreateCpuBackend(numThreads);
    }
    return new NDict::NMT::NYNMT::TGpuBackend(deviceIndex);
}

template <typename TFloatType>
class TYNMTApplier: public IBertApplier<TFloatType> {
public:
    TYNMTApplier(int maxBatchSize, int maxInputLength, const TString& modelPath, NDict::NMT::NYNMT::TBackendPtr backend)
        : MaxInputLength(maxInputLength)
        , ModelBlob(TBlob::FromFile(modelPath))
        , Backend(backend)
        , InitEnv(Backend.Backend.Get(), {}, false, false)
        , Bert(maxBatchSize, maxInputLength, 0)
    {
        auto head = MakeHolder<TRegressionHead<TFloatType>>(
            maxBatchSize,
            InitEnv,
            NDict::NMT::NYNMT::ReadModelProtoWithMeta(ModelBlob));
        Bert.Initialize(ModelBlob, InitEnv, Backend, std::move(head));
    }

    TVector<TFloatType> ProcessBatch(const TConstArrayRef<TVector<int>>& batch) override {
        NBertApplier::TBertInput bertInput(batch.size(), MaxInputLength);
        for (const auto& sample : batch) {
            bertInput.Add(sample);
        }
        return Bert.ProcessBatch(bertInput);
    }

private:

    int MaxInputLength;
    TBlob ModelBlob;
    NDict::NMT::NYNMT::TBackendWithMemory Backend;
    NDict::NMT::NYNMT::TInitializationEnvironment InitEnv;
    TBertWrapper<TFloatType, TRegressionHead> Bert;
};

template <typename TFloatType>
TVector<TFloatType> Run(const TVector<TVector<int>>& data,
         const TVector<TString>& samples,
         int maxBatchSize,
         int maxInputLength,
         const TString& weightsFilename,
         NDict::NMT::NYNMT::TBackendPtr backend) {
    THolder<IBertApplier<TFloatType>> bertApplier;
    bertApplier.Reset(new TYNMTApplier<TFloatType>(maxBatchSize, maxInputLength, weightsFilename, backend));

    typename TRegressionHead<TFloatType>::TResult allResults;

    TConstArrayRef<TVector<int>> warmupBatch(data.begin(), data.begin() + Min<size_t>(data.size(), maxBatchSize));
    bertApplier->ProcessBatch(warmupBatch);

    for (auto i = 0U; i < (samples.size() - 1) / maxBatchSize + 1; ++i) {
        size_t start = i * maxBatchSize;
        auto end = Min(start + maxBatchSize, data.size());

        const int batchSize = end - start;
        Y_ENSURE(batchSize <= maxBatchSize, "Maximum batch size exceeded: " << batchSize << " > " << maxBatchSize);

        TConstArrayRef<TVector<int>> batch(data.begin() + start, data.begin() + end);

        auto result = bertApplier->ProcessBatch(batch);
        allResults.insert(allResults.end(), result.begin(), result.end());
    }

    return allResults;
}

template <typename TFloatType>
TVector<TFloatType> Run(const TVector<TVector<int>>& data,
         const TVector<TString>& samples,
         int maxBatchSize,
         int maxInputLength,
         TMaybe<size_t> numThreads,
         const TString& weightsFilename,
         bool useCpu,
         int deviceIndex) {
    return Run<TFloatType>(data, samples, maxBatchSize, maxInputLength, weightsFilename, CreateBackend(useCpu, deviceIndex, numThreads));
}

