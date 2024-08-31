#pragma once

#include <alice/boltalka/generative/service/server/handlers/request_handler.h>

#include <alice/boltalka/generative/service/proto/bert_request.pb.h>
#include <alice/boltalka/generative/service/proto/bert_response.pb.h>
#include <alice/boltalka/generative/service/server/config/config.pb.h>

#include <alice/boltalka/libs/text_utils/utterance_transform.h>
#include <library/cpp/langs/langs.h>

#include <kernel/bert/batch_processor.h>
#include <kernel/bert/tokenizer.h>

#include <util/folder/path.h>

namespace NGenerativeBoltalka {

struct TBertOutputParams {
    float Scale;
    float Bias;
    bool IsTargetCe;
    bool DoBinarization;
    float BinarizeThreshold;
};

using TBaseBertRequestHandler = IRequestHandler<Proto::TBertFactorRequest, Proto::TBertFactorResponse>;

template <template<typename> class TEncoderHead>
class TBertRequestHandler : public TBaseBertRequestHandler {
private:
    void Tokenize(const TString& utterance, TVector<TVector<TUtf32String>>* tokenIds) const;
    void TruncateUniformly(TVector<TVector<TUtf32String>>* tokenIds) const;
    void TruncateDialogue(TVector<TVector<TUtf32String>>* contextTokenIds, TVector<TVector<TUtf32String>>* candidatesTokenIds, size_t maxCandidateLength, bool allowSegmentBreak = false) const;
    const NBertApplier::TBertIdConverter Converter;
    const NBertApplier::TBertSegmenter Segmenter;
    const TIntrusivePtr<ITimeProvider> TimeProvider;
    NBertApplier::TBertModel<TFloat16, TEncoderHead> Model;
    NBertApplier::TBatchProcessor<TFloat16, TEncoderHead> Processor;
    const NNlgTextUtils::TNlgSearchUtteranceTransform Transform;
    size_t ContextLength;
    bool TruncateAsDialogue;
    const THashMap<size_t, TBertOutputParams> TargetToParams;
    const THashMap<size_t, size_t> TargetToOutputIdx;
public:
    using TResult = typename decltype(Processor)::TModelResult;

    TBertRequestHandler(const TConfig::TBertFactor& config);
    TBertRequestHandler(const TFsPath& folder, size_t batchSize, size_t maxInputLen, int gpuId, size_t contextLen, bool truncateAsDialogue, const THashMap<size_t, TBertOutputParams>& targetToParams);

    Proto::TBertFactorResponse HandleRequest(const Proto::TBertFactorRequest& request) override;
    NThreading::TFuture<TResult> ProcessSample(const TVector<TString>& sample);
private:
    TVector<NThreading::TFuture<TResult>> ProcessMultipleSamples(const TVector<TString>& context, const TVector<TString>& candidates);
};

} // namespace NGenerativeBoltalka
