#include "bert_factor_request_handler.h"
#include <dict/mt/libs/nn/ynmt_backend/gpu_if_supported/backend.h>
#include <dict/mt/libs/nn/ynmt/extra/encoder_head.h>

namespace NGenerativeBoltalka {

namespace {

constexpr int CLS = 3;
constexpr int SEP = 4;

enum EBertTargets {
    BERT_RELEV,
    BERT_INTEREST,
    BERT_NOT_MALE,
    BERT_NOT_RUDE,
    BERT_RESPECT,
    BERT_INFORMATIVENESS,
    BERT_LAST
};

TBertOutputParams ParseBertOutputParamsProto(const TConfig::TBertFactor::TOutputParams& proto) {
    return {
        proto.GetScale(),
        proto.GetBias(),
        proto.GetTargetType() == TConfig_TBertFactor_ETargetType_Ce,
        proto.GetDoBinarization(),
        proto.GetBinarizeThreshold()
    };
}

THashMap<size_t, TBertOutputParams> GetTargetToParamsMapping(const TConfig::TBertFactor& config) {
    return {
        {BERT_RELEV, ParseBertOutputParamsProto(config.GetRelevParams())},
        {BERT_INTEREST, ParseBertOutputParamsProto(config.GetInterestParams())},
        {BERT_NOT_MALE, ParseBertOutputParamsProto(config.GetNotMaleParams())},
        {BERT_NOT_RUDE, ParseBertOutputParamsProto(config.GetNotRudeParams())},
        {BERT_RESPECT, ParseBertOutputParamsProto(config.GetRespectParams())},
        {BERT_INFORMATIVENESS, ParseBertOutputParamsProto(config.GetInformativenessParams())},
    };
}

THashMap<size_t, size_t> GetTargetToOutputIdxMapping(const THashMap<size_t, TBertOutputParams>& targetToParams) {
    THashMap<size_t, size_t> targetToOutputIdx;
    size_t outputIdx = 0;
    for (size_t targetIdx = BERT_RELEV; targetIdx < BERT_LAST; ++targetIdx) {
        const auto& params = targetToParams.find(targetIdx);
        Y_ENSURE(params != targetToParams.end());
        outputIdx += params->second.IsTargetCe;
        targetToOutputIdx[targetIdx] = outputIdx++;
    }
    return targetToOutputIdx;
}

float TransformScore(float score, const TBertOutputParams& params) {
    if (params.DoBinarization) {
        score = score > params.BinarizeThreshold ? 1.0 : 0.0;
    }
    return score * params.Scale + params.Bias;
}

NDict::NMT::NYNMT::TBackendPtr CreateBackend(int deviceIndex) {
    if (NDict::NMT::NYNMT::IsGpuBackendSupported()) {
        return *NDict::NMT::NYNMT::CreateGpuBackendIfSupported(deviceIndex);
    }
    return new NDict::NMT::NYNMT::TCpuBackend();
}

};

template <template<typename> class TEncoderHead>
TBertRequestHandler<TEncoderHead>::TBertRequestHandler(const TConfig::TBertFactor& config)
        : TBertRequestHandler::TBertRequestHandler(config.GetFolder(), config.GetBatchSize(), config.GetMaxInputLen(), config.GetGpuIds()[0], config.GetContextLen(), config.GetTruncateAsDialogue(), GetTargetToParamsMapping(config))
    {
        Y_VERIFY(config.GetGpuIds().size() == 1, "Only one GPU is supported for BertFactor so far");
    }

template <template<typename> class TEncoderHead>
TBertRequestHandler<TEncoderHead>::TBertRequestHandler(const TFsPath& folder, size_t batchSize, size_t maxInputLen, int gpuId, size_t contextLen, bool truncateAsDialogue, const THashMap<size_t, TBertOutputParams>& targetToParams)
        : Converter(folder / "vocab.txt")
        , Segmenter(folder / "start.trie", folder / "cont.trie")
        , TimeProvider(CreateDefaultTimeProvider())
        , Model(folder / "boltalka-bert-mse.npz"
                , batchSize
                , maxInputLen
                , CreateBackend(gpuId))
        , Processor(Model, *TimeProvider)
        , Transform(LanguageByName("ru"))
        , ContextLength(contextLen)
        , TruncateAsDialogue(truncateAsDialogue)
        , TargetToParams(targetToParams)
        , TargetToOutputIdx(GetTargetToOutputIdxMapping(targetToParams))
    {
        Y_ASSERT(Model.GetMaxInputLength() >= ContextLength + 3);
    }

template <template<typename> class TEncoderHead>
void TBertRequestHandler<TEncoderHead>::TruncateDialogue(TVector<TVector<TUtf32String>>* contextTokenIds, TVector<TVector<TUtf32String>>* candidatesTokenIds, size_t maxCandidateLength, bool allowSegmentBreak) const {
    const size_t maxTextLength = Model.GetMaxInputLength() - ContextLength - 3;
    size_t currentLength = maxCandidateLength;
    for (const auto& tokenIds : *contextTokenIds) {
        currentLength += tokenIds.size();
    }
    if (currentLength <= maxTextLength) {
        return;
    }
    size_t cropLength = currentLength - maxTextLength;
    size_t segmentId = 0;
    while (cropLength > 0 && segmentId + 1 < contextTokenIds->size()) {
        size_t segmentLen = (*contextTokenIds)[segmentId].size();
        if (cropLength >= segmentLen || !allowSegmentBreak) {
            (*contextTokenIds)[segmentId].clear();
            cropLength -= Min(cropLength, segmentLen);
        } else {
            (*contextTokenIds)[segmentId].erase((*contextTokenIds)[segmentId].begin(), (*contextTokenIds)[segmentId].begin() + cropLength);
            cropLength = 0;
        }
        ++segmentId;
    }
    if (cropLength == 0) {
        return;
    }
    // truncate context_0 and reply
    size_t newContextLength = contextTokenIds->back().size();
    if (contextTokenIds->back().size() > maxTextLength / 2) {
        newContextLength = Max(maxTextLength / 2, newContextLength - cropLength);
    }
    size_t newCandidateLength = maxTextLength - newContextLength;
    contextTokenIds->back().erase(contextTokenIds->back().begin(), contextTokenIds->back().end() - newContextLength);
    for (auto& tokenIds : *candidatesTokenIds) {
        tokenIds.resize(newCandidateLength);
    }
}

template <template<typename> class TEncoderHead>
void TBertRequestHandler<TEncoderHead>::TruncateUniformly(TVector<TVector<TUtf32String>>* tokenIds) const {
    const size_t maxSegmentLength = (Model.GetMaxInputLength() - ContextLength - 3) / (ContextLength + 1); // truncate each turn to equal fraction of length excluding BOS, [CLS], [SEP]
    for (auto& segmentTokenIds : *tokenIds) {
        segmentTokenIds.resize(Min(segmentTokenIds.size(), maxSegmentLength));
    }
}

template <template<typename> class TEncoderHead>
void TBertRequestHandler<TEncoderHead>::Tokenize(const TString& utterance, TVector<TVector<TUtf32String>>* tokens) const {
    TString processedUtterance = Transform.Transform(utterance);
    const auto utf32Text = TUtf32String::FromUtf8(processedUtterance);
    tokens->emplace_back(Segmenter.Split(utf32Text));
}

template <template<typename> class TEncoderHead>
TVector<NThreading::TFuture<typename TBertRequestHandler<TEncoderHead>::TResult>> TBertRequestHandler<TEncoderHead>::ProcessMultipleSamples(const TVector<TString>& context, const TVector<TString>& candidates) {
    TVector<TVector<TUtf32String>> contextTokens;
    for (const auto& val : context) {
        Tokenize(val, &contextTokens);
    }

    TVector<TVector<TUtf32String>> candidatesTokens;
    size_t maxCandidateLength = 0;
    for (const auto& val : candidates) {
        Tokenize(val, &candidatesTokens);
        maxCandidateLength = Max(maxCandidateLength, candidatesTokens.back().size());
    }

    if (TruncateAsDialogue) {
        TruncateDialogue(&contextTokens, &candidatesTokens, maxCandidateLength);
    } else {
        TruncateUniformly(&contextTokens);
        TruncateUniformly(&candidatesTokens);
    }

    TVector<TVector<int>> contextTokenIds;
    contextTokenIds.reserve(contextTokens.size());
    for (const auto& val : contextTokens) {
        contextTokenIds.emplace_back(Converter.Convert(val));
    }
    TVector<TVector<int>> candidatesTokenIds;
    candidatesTokenIds.reserve(candidatesTokens.size());
    for (const auto& val : candidatesTokens) {
        candidatesTokenIds.emplace_back(Converter.Convert(val));
    }

    TVector<int> bertInput;
    bertInput.reserve(ContextLength + 3 + contextTokenIds.size() + maxCandidateLength);
    bertInput.push_back(CLS);
    for (size_t i = contextTokenIds.size(); i < ContextLength; ++i) {
        bertInput.push_back(SEP);
    }
    for (const auto& segmentTokenIds : contextTokenIds) {
        std::move(segmentTokenIds.begin(), segmentTokenIds.end(), std::back_inserter(bertInput));
        bertInput.push_back(SEP);
    }

    size_t truncateSize = bertInput.size();

    TVector<NThreading::TFuture<TResult>> results;
    results.reserve(candidatesTokenIds.size());
    for (const auto& candidateTokenIds : candidatesTokenIds) {
        bertInput.resize(truncateSize);
        std::move(candidateTokenIds.begin(), candidateTokenIds.end(), std::back_inserter(bertInput));
        bertInput.push_back(SEP);
        results.push_back(Processor.Process({bertInput.data(), bertInput.size()}, /* forceAsync */ true));
    }
    return results;
}

template <template<typename> class TEncoderHead>
Proto::TBertFactorResponse TBertRequestHandler<TEncoderHead>::HandleRequest(const Proto::TBertFactorRequest& request) {
    TVector<TString> context;
    for (const auto& val : request.GetContext()) {
        context.push_back(val);
    }
    if (context.size() > ContextLength) {
        context.erase(context.begin(), context.end() - ContextLength);
    }
    TVector<TString> candidates;
    for (const auto& val : request.GetCandidates()) {
        candidates.push_back(val);
    }
    auto results = ProcessMultipleSamples(context, candidates);
    Proto::TBertFactorResponse response;
    response.MutableCandidatesScores()->Reserve(results.size());
    for (const auto& r : results) {
        auto values = r.GetValueSync();
        // TODO: remove unused CE outputs
        for (const auto& [targetIdx, outputIdx] : TargetToOutputIdx) {
            Y_ENSURE(outputIdx < values.size());
            values[outputIdx] = TransformScore(values[outputIdx], TargetToParams.at(targetIdx));
        }
        Proto::TBertOutput bertOutput;
        bertOutput.SetRelevScore(values[TargetToOutputIdx.at(BERT_RELEV)]);
        bertOutput.SetInterestScore(values[TargetToOutputIdx.at(BERT_INTEREST)]);
        bertOutput.SetNotMaleScore(values[TargetToOutputIdx.at(BERT_NOT_MALE)]);
        bertOutput.SetNotRudeScore(values[TargetToOutputIdx.at(BERT_NOT_RUDE)]);
        bertOutput.SetRespectScore(values[TargetToOutputIdx.at(BERT_RESPECT)]);
        bertOutput.SetInformativenessScore(values[TargetToOutputIdx.at(BERT_INFORMATIVENESS)]);
        std::move(values.begin(), values.end(), RepeatedFieldBackInserter(bertOutput.MutableScores()));
        *response.AddCandidatesScores() = std::move(bertOutput);
    }
    return response;
}

template <template<typename> class TEncoderHead>
NThreading::TFuture<typename TBertRequestHandler<TEncoderHead>::TResult> TBertRequestHandler<TEncoderHead>::ProcessSample(const TVector<TString>& sample) {
    TVector<TString> context;
    TVector<TString> candidates;
    if (sample.empty()) {
        context = { TString() };
        candidates = { TString() };
    } else {
        std::copy(sample.begin(), sample.end() - 1, std::back_inserter(context));
        candidates.push_back(sample.back());
    }
    auto results = ProcessMultipleSamples(context, candidates);
    Y_ASSERT(results.size() == 1);
    return results.back();
}

template class TBertRequestHandler<NDict::NMT::NYNMT::TRegressionHead>;
template class TBertRequestHandler<NDict::NMT::NYNMT::TClassificationHead>;
template class TBertRequestHandler<NDict::NMT::NYNMT::TMultitargetHead>;

} // namespace NGenerativeBoltalka
