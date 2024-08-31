#include "model.h"

#include "util.h"

#include <dict/dictutil/str.h>
#include <dict/mt/libs/batch_executor/batch_executor.h>
#include <dict/mt/libs/nn/beam_search/constraint.h>
#include <dict/mt/libs/nn/beam_search/banned_tokens_constraint.h>
#include <dict/mt/libs/nn/beam_search/generate_shorter_constraint.h>
#include "dict/mt/libs/nn/beam_search/max_length_constraint.h"
#include <dict/mt/libs/nn/beam_search/min_length_constraint.h>
#include <dict/mt/libs/nn/beam_search/repeat_constraint.h>
#include <dict/mt/libs/nn/beam_search/sawtooth_constraint.h>
#include <dict/mt/libs/nn/beam_search/span_generation_constraint.h>
#include <dict/mt/libs/nn/ynmt/encdec/encdec.h>
#include <dict/mt/libs/nn/ynmt_backend/cpu/backend.h>
#ifdef HAVE_CUDA
#include <dict/mt/libs/nn/ynmt_backend/gpu/backend.h>
#endif

#include <library/cpp/iterator/concatenate.h>
#include <library/cpp/threading/future/async.h>

#include <util/digest/sequence.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/xrange.h>
#include <util/memory/blob.h>

using namespace NDict::NMT;
using namespace NDict::NMT::NYNMT;

namespace NGenerativeBoltalka {
    namespace {

        std::vector<TBackendWithMemoryPtr> CreateBackends(const TGenerativeModel::TParams& modelParams) {

            std::vector<TBackendWithMemoryPtr> backends;
            if (modelParams.ShouldUseGpus) {
#ifdef HAVE_CUDA
                for (size_t gpuId : modelParams.GpuIds) {
                    backends.emplace_back(new TBackendWithMemory(new TGpuBackend(gpuId, 0, TGpuBackend::TGpuBackendRngAlgorithm::CudaFast)));
                }
#else
                Y_FAIL("The program was compiled without GPU support");
#endif
            } else {
                for (size_t i = 0; i < modelParams.NumCpuBackends; ++i) {
                    backends.emplace_back(new TBackendWithMemory(CreateCpuBackend(modelParams.NumThreadsPerSession)));
                }
            }

            return backends;
        }

        TBlob LoadModelBlob(const TGenerativeModel::TParams& modelParams) {
            return modelParams.CheckpointMemoryMode == EMemoryMode::Locked
                ? TBlob::LockedFromFile(modelParams.CheckpointPath)
                : TBlob::PrechargedFromFile(modelParams.CheckpointPath);
        }

        std::pair<std::vector<std::unique_ptr<IBatchNeuralEncoder>>, int> CreateEncodersAndCalcMaxOutLen(
            const TGenerativeModel::TParams& modelParams,
            const TBlob& modelBlob
        ) {
            std::vector<TBackendWithMemoryPtr> backends = CreateBackends(modelParams);

            TNeuralEncdecParams encdecParams;
            if (modelParams.UsePrefixedInference) {
                encdecParams.MaxInpLen = 1;
                encdecParams.MaxOutLen = modelParams.MaxOutLen + 2 + modelParams.MaxInpLen; // +2 since in ynmt the (MaxOutputLen - 2) is the largest valid hypothesis
            } else {
                encdecParams.MaxInpLen = modelParams.MaxInpLen;
                encdecParams.MaxOutLen = modelParams.MaxOutLen + 2; // +2 since in ynmt the (MaxOutputLen - 2) is the largest valid hypothesis
            }
            encdecParams.MaxBatchSize = modelParams.BatchSize;
            encdecParams.MaxHypos = modelParams.BeamSize * modelParams.BatchSize;
            encdecParams.MaxBeamSize = modelParams.BeamSize;
            encdecParams.DirectDeploy = true;
            encdecParams.MaxPtuneLen = modelParams.MaxPtuneLen;

            if (modelParams.SamplingStrategy == "sampling") {
                encdecParams.DefaultSamplerParams.Mode = TSamplerParams::EMode::Sampling;
                encdecParams.DefaultSamplerParams.TopNLogits = modelParams.SamplingTopNLogits;
                encdecParams.DefaultSamplerParams.Temperature = modelParams.SamplingTemperature;
                encdecParams.DefaultSamplerParams.NucleusSampling = modelParams.SamplingNucleus;
            } else if (modelParams.SamplingStrategy == "beam_search") {
                encdecParams.DefaultSamplerParams.Mode = TSamplerParams::EMode::BeamSearch;
            } else if (modelParams.SamplingStrategy == "stochastic_beam_search") {
                encdecParams.DefaultSamplerParams.Mode = TSamplerParams::EMode::StochasticBeamSearch;
                encdecParams.DefaultSamplerParams.TopNLogits = modelParams.SamplingTopNLogits;
                encdecParams.DefaultSamplerParams.Temperature = modelParams.SamplingTemperature;
                encdecParams.DefaultSamplerParams.NucleusSampling = modelParams.SamplingNucleus;
            } else {
                Y_VERIFY(false, "Sampling strategy should be either `sampling`, `beam_search` or `stochastic_beam_search`");
            }

            return std::make_pair(CreateTransformerEncoders(backends, modelBlob, encdecParams), encdecParams.MaxOutLen);
        }

        NDict::NMT::TNeuralEncdecParams CreateNeuralEncdecParamsForScorer(const TGenerativeModel::TParams& modelParams) {
            return NDict::NMT::TNeuralEncdecParams{
                .MaxInpLen = 1,
                .MaxOutLen = static_cast<int>(modelParams.MaxInpLen),
                .MaxHypos = static_cast<int>(modelParams.BatchSize),
                .MaxBatchSize = static_cast<int>(modelParams.BatchSize),
                .MaxBeamSize = 1,
                .MaxPtuneLen = static_cast<int>(modelParams.MaxPtuneLen),
                .DirectDeploy = true,
                .ConvertFloat16ToFloat32 = false,
                .ForceFloat32Logits = true
            };
        }

        TVector<TGenerativeResponse> PrepareResponses(const TVector<int>& inputIds, const TVector<TTranslation>& translations, const TGenerativeModel* model, const TTokenizer* tokenizer, TPostProcessor* postProcessor) {
            TVector<TGenerativeResponse> responses(Reserve(translations.size()));
            for (const auto& translation : translations) {
                std::vector<int> tokenIds(translation.Ids);
                model->MaybeErasePrefix(inputIds, &tokenIds);
                if (tokenIds.empty()) {
                    continue;
                }

                TString response = tokenizer->Detokenize(tokenIds);

                auto postProcessedResponse = postProcessor->PostProcessText(response);

                size_t numTokens = tokenIds.size();
                auto scorePtr = translation.Scores.FindPtr("ynmt");
                Y_ENSURE(scorePtr, "Score cannot be null");
                float score = model->CalculateNormalizedScore(numTokens, *scorePtr);
                responses.push_back(TGenerativeResponse(postProcessedResponse, score, numTokens));
            }
            return responses;
        }

    } // namespace anonymous

    // TGenerativeModel --------------------------------------------------------------------
    TGenerativeModel::TGenerativeModel(const TGenerativeModel::TParams& params, const TTokenizer::TParams& tokenizerParams, const TString& pheadPath)
        : Params(params)
    {
        TBlob modelBlob = LoadModelBlob(Params);
        if (Params.ShouldInitializeTranslator) {
            TranslatorManager = CreateSharedNeuralTranslatorManager();
            Translator = CreateTranslator(modelBlob, TranslatorManager.get());
        }

        if (Params.ShouldInitializeScorer || Params.ShouldInitializeEmbedder || Params.ShouldInitializePHead) {
            Scorer = CreateScorer(modelBlob, pheadPath);
        }

        auto tokenizerParamsCopy = tokenizerParams;
        if (TNpzFile modelFile(modelBlob); modelFile.Has("bpe.voc")) {
            tokenizerParamsCopy.BpeVocContent = modelFile.GetRawData("bpe.voc");
        }
        Tokenizer = GetTokenizer(tokenizerParamsCopy);
        BannedTokenIdsToGenerate = PrepareBannedTokenIds(params.BannedTokensToGenerate,
                                                         params.FileForBannedTokensToGenerate,
                                                         params.BannedTokenIdsToGenerate,
                                                         params.FileForBannedTokenIdsToGenerate,
                                                        params.AllowBanUnkTokenToGenerate);
    }

    std::unique_ptr<NDict::NMT::ITranslator> TGenerativeModel::CreateTranslator(
            const TBlob& modelBlob, NDict::NMT::ISharedNeuralTranslatorManager* translatorManager) {

        auto[encoders, maxOutLen] = CreateEncodersAndCalcMaxOutLen(Params, modelBlob);

        std::vector<ISharedNeuralTranslatorManager::TEncoderInstance> encoderInstances;
        for (size_t i = 0; i < encoders.size(); ++i) {
            encoderInstances.emplace_back(
                ISharedNeuralTranslatorManager::TEncoderInstance{std::move(encoders[i]), {static_cast<int>(-i - 1)}}
            );
        }

        TNeuralTranslatorParams neuralParams;
        neuralParams.DstEosId = Params.DstEosId;
        neuralParams.DefaultBeamSize = Params.BeamSize;
        neuralParams.BeamSpread = 1.e4;  // effectively no beam spread
        neuralParams.LenAlpha = 0.0; // no length penalty
        neuralParams.AttnBeta = 0.0;
        neuralParams.MaxBatchSize = Params.BatchSize;
        neuralParams.MaxHypos = Params.BeamSize * Params.BatchSize;
        neuralParams.SuggestMaxLength = 100;
        neuralParams.TimeoutMsModel = Params.MaxGenerationMsModel;
        neuralParams.TimeoutMsExecutor = Params.MaxGenerationMsExecutor;
        neuralParams.ShouldTerminateBeamAfterFinishing = true;
        neuralParams.IncludeLateHypotheses = Params.IncludeLateHypotheses;
        neuralParams.ReturnUnfinishedHypotheses = Params.ReturnUnfinishedHypotheses;

        neuralParams.MinOutputLen = Params.MinOutLen.GetOrElse(0);
        neuralParams.MaxOutputLen = maxOutLen;

        return translatorManager->CreateNeuralTranslator(std::move(encoderInstances), neuralParams);
    }

    std::unique_ptr<NDict::NMT::NYNMT::TDecoderLMScorer> TGenerativeModel::CreateScorer(const TBlob& modelBlob, const TString& pheadPath) {
        std::vector<TBackendWithMemoryPtr> backends = CreateBackends(Params);
        NDict::NMT::NYNMT::THeadParams headParams;
        headParams.HeadMode = Params.HeadMode;
        headParams.MaxHeadSize = Params.MaxLinearHeadOutputDim;
        if (!pheadPath.empty() && Params.HeadMode == EHeadMode::Linear) {
            headParams.Proto = ReadModelProtoWithMeta(TBlob::FromFile(pheadPath));
        } else {
            headParams.Proto = Nothing(),
            headParams.NormalizeOutTokensRepresentations = Params.NormalizeOutTokensRepresentations;
        }
        if (Params.HeadWeightsName) {
            headParams.HeadWeightsName = Params.HeadWeightsName;
        }

        return std::unique_ptr<NDict::NMT::NYNMT::TDecoderLMScorer> (new NDict::NMT::NYNMT::TDecoderLMScorer {
            backends,
            modelBlob,
            CreateNeuralEncdecParamsForScorer(Params),
            headParams
        } );
    }

    TVector<TTranslation> TGenerativeModel::InferModel(const TVector<int>& tokenIds,
                                                       int numHypos,
                                                       TMaybe<int> seed,
                                                       bool prefixTokensOnly,
                                                       const TVector<TVector<int>>& spanDelimiters,
                                                       bool diverseBeamSearch) const {
        return InferModel({
            .InputIds = tokenIds,
            .Seed = seed,
            .NumHypos = static_cast<size_t>(numHypos),
            .PrefixOnly = prefixTokensOnly,
            .SpanDelimitersIds = spanDelimiters,
            .DiverseBeamSearch = diverseBeamSearch});
    }

    TVector<TVector<TTranslation>> TGenerativeModel::InferModel(const TVector<TGenerativeRequest>& requestsBatch) const {
        std::vector<TTranslationSrc> batchSrcs;
        for (const auto& request : requestsBatch) {
            const auto translationSrc = MakeSrcFromRequest(request);

            for (auto i : xrange(request.NumHypos)) {
                auto curTranslationSrc = translationSrc;
                if (request.Seed.Defined()) {
                    // If seed is specified, then we hash the whole input with the seed in order to set the real seed to the random generator inside
                    curTranslationSrc.Seed = CombineHashes(TSimpleRangeHash{}(request.InputIds), ::THash<size_t>()(*request.Seed + i));
                }
                batchSrcs.push_back(curTranslationSrc);
            }
        }

        const auto translations = Translator->TranslateMany(batchSrcs);
        if (translations.size() != batchSrcs.size()) {
            ythrow yexception() << "Translator returned fewer translations " << translations.size() << " than requested " << batchSrcs.size();
        }

        TVector<TVector<TTranslation>> results;
        results.reserve(requestsBatch.size());
        auto translIt = translations.begin();
        for (const auto& request : requestsBatch) {
            results.emplace_back();
            auto inserter = std::back_inserter(results.back());

            for ([[maybe_unused]] auto i : xrange(request.NumHypos)) {
                Copy(std::make_move_iterator(translIt->begin()), std::make_move_iterator(translIt->end()), inserter);
                ++translIt;
            }
        }
        return results;
    }

    TVector<TTranslation> TGenerativeModel::InferModel(const TGenerativeRequest& request) const {
        const auto result = InferModel(TVector{request});
        Y_ENSURE(!result.empty());
        return result.front();
    }

    std::vector<float> TGenerativeModel::Score(TVector<int> inputIds, const TMaybe<NDict::NMT::TPtuneBlock>& ptuneBlock) const {
        if (Scorer.get() == nullptr) {
            ythrow yexception() << "Could not apply scorer, got nullptr instead";
        }

        TDecoderLMScorer::TScorerSrc src{std::vector<int>{inputIds.begin(), inputIds.end() - 1},
                                         ptuneBlock,
                                         std::vector<int>{inputIds.begin() + 1, inputIds.end()}};

        std::vector<float> result = Scorer->Score(src, TInstant::Max());
        return result;
    }

    std::vector<std::vector<float>> TGenerativeModel::Embed(TVector<int> inputIds,
                                               const TMaybe<NDict::NMT::TPtuneBlock>& ptuneBlock,
                                               TModelProtoWithMeta* pHead) const {
        if (Scorer.get() == nullptr) {
            ythrow yexception() << "Could not apply embedder, got nullptr instead";
        }

        TDecoderLMScorer::TScorerSrc src{.Ids = std::vector<int>{inputIds.begin(), inputIds.end()},
                                         .PtuneBlock = ptuneBlock,
                                         .ScoringIds = {},
                                         .PHead = pHead,
                                         .PoolingToken = inputIds.size() - 1,
                                         .ReturnFullEmbMatrix = Params.ReturnFullEmbMatrix};

        std::vector<float> concatEmbs = Scorer->Embed(src, TInstant::Max());
        if (!Params.ReturnFullEmbMatrix) {
            return {concatEmbs};
        } else {
            size_t nTokens = inputIds.size();
            Y_ENSURE(concatEmbs.size() % nTokens == 0, "Expected output size divisible by input size");
            std::vector<std::vector<float>> embs;
            size_t embSize = concatEmbs.size() / nTokens;
            for (size_t i = 0; i < nTokens; ++i) {
                embs.emplace_back(concatEmbs.begin() + i * embSize,
                                  concatEmbs.begin() + (i + 1) * embSize);
            }
            return embs;
        }
    }

    std::vector<float> TGenerativeModel::ScoreSingleTokenSuffixes(const TVector<int>& prefixIds,
                                                                  const TVector<int>& suffixesIds,
                                                                  const TMaybe<NDict::NMT::TPtuneBlock>& ptuneBlock) {
        if (Scorer.get() == nullptr) {
            ythrow yexception() << "Could not apply scorer, got nullptr instead";
        }

        TDecoderLMScorer::TScorerSrc src{std::vector<int>{prefixIds.begin(), prefixIds.end()},
                                         ptuneBlock,
                                         std::vector<int>{prefixIds.begin(), prefixIds.end()}};

        std::vector<std::vector<float>> logits = Scorer->GetLogits(src, TInstant::Max());
        auto lastTokenLogits = logits.back();

        auto suffixesCount = suffixesIds.size();
        std::vector<float> suffixesScores;
        suffixesScores.reserve(suffixesCount);
        for (auto suffixId: suffixesIds) {
            suffixesScores.push_back(lastTokenLogits[suffixId]);
        }

        return suffixesScores;
    }

    void TGenerativeModel::MaybeErasePrefix(const TVector<int>& inputIds, std::vector<int>* tokenIds) const {
        if (Params.UsePrefixedInference) {
            if (tokenIds->size() > inputIds.size()) {
                tokenIds->erase(tokenIds->begin(), tokenIds->begin() + inputIds.size());
            } else {
                tokenIds->clear();
            }
        }
    }

    float TGenerativeModel::CalculateNormalizedScore(const size_t numTokens, const float score) const {
        if (!Params.UsePrefixedInference && numTokens < 2) {
            return 0.0f;
        }

        auto tokensToCount = Params.UsePrefixedInference ? numTokens : numTokens - 1;  // in case of standard transformer we do not want to count _BOS_ in score
        return score / float(tokensToCount);
    }

    TVector<std::unique_ptr<NDict::NMT::IBeamSearchConstraint>> TGenerativeModel::ConfigureConstraints(const TGenerativeRequest& request) const {
        TVector<std::unique_ptr<NDict::NMT::IBeamSearchConstraint>> constraints;
        auto minOutLen = request.MinOutLen.Defined() ? request.MinOutLen : Params.MinOutLen;
        if (minOutLen.Defined() && minOutLen.GetRef() > 2) {
            constraints.push_back(std::make_unique<NDict::NMT::TMinLengthConstraint>(
                    minOutLen.GetRef() - 2, Params.DstEosId));
        }

        for (const auto& penaltyParam: Params.NGramPenaltyParams) {
            constraints.push_back(std::make_unique<NDict::NMT::TRepeatConstraint>(penaltyParam));
        }

        if (BannedTokenIdsToGenerate.size() > 0) {
            constraints.push_back(std::make_unique<NDict::NMT::TBannedTokensConstraint>(
                    std::make_unique<THashSet<int>>(BannedTokenIdsToGenerate)));
        }

        if (request.PrefixOnly) {
            constraints.push_back(std::make_unique<NDict::NMT::TSpanGenerationConstraint>(
                Params.DstEosId,
                std::make_unique<TVector<TVector<int>>>(request.SpanDelimitersIds.begin(), request.SpanDelimitersIds.end())));
        }

        auto maxOutLen = request.MaxOutLen.Defined() ? std::min(Params.MaxOutLen, request.MaxOutLen.GetRef()) : Params.MaxOutLen;
        constraints.push_back(std::make_unique<NDict::NMT::TMaxLengthConstraint>(maxOutLen, Params.DstEosId)); // Hard penalty
        if (Params.GenerateShorter && Params.BoostEos.Defined()) {
            constraints.push_back(std::make_unique<NDict::NMT::TGenerateShorterConstraint>(Params.MaxOutLen, Params.DstEosId, Params.BoostEos.GetRef())); // Soft penalty
        }

        if (Params.SawtoothBoost) {
            constraints.push_back(std::make_unique<NDict::NMT::TSawtoothConstraint>(Params.SawtoothTokenId, Params.SawtoothBoostMin, Params.SawtoothBoostStep,
                    Params.SawtoothBoostMax, Params.SawtoothConsumePrefix));
        }

        return constraints;
    }

    THashSet<int> TGenerativeModel::PrepareBannedTokenIds(const TVector<TString>& bannedTokensToGenerate,
                                                          const TMaybe<TString>& fileForBannedTokensToGenerate,
                                                          const TVector<int>& bannedTokenIdsToGenerate,
                                                          const TMaybe<TString>& fileForBannedTokenIdsToGenerate,
                                                          bool allowUnkBanning) const {
        THashSet<int> result;
        for (auto& tokenId : Concatenate(
                bannedTokenIdsToGenerate,
                fileForBannedTokenIdsToGenerate.Defined() ? LoadFileToHashSet<int>(fileForBannedTokenIdsToGenerate.GetRef()) : THashSet<int>())) {
            result.insert(tokenId);
        }

        for (auto& tokenString : Concatenate(
                bannedTokensToGenerate,
                fileForBannedTokensToGenerate.Defined() ? LoadFileToHashSet<TString>(fileForBannedTokensToGenerate.GetRef()) : THashSet<TString>())) {
            int tokenId = Tokenizer->TokenId(tokenString);

            if (tokenId == Tokenizer->Unk()) {
                Y_ENSURE(allowUnkBanning, TStringBuilder() << "You tried to ban token `"
                                                           << tokenString
                                                           << "` which translated to UNK token in the vocabulary "
                                                           << "but you did not allowed explicitly to bun UNK tokens. "
                                                           << "Which means you likely tried to ban non-existing token.");
            }
            result.insert(tokenId);
        }

        for (auto& tokenId : result) {
            if (tokenId == Tokenizer->Eos()) {
                Cerr << "Warning: you tried to ban EOS token. Please ensure that this is intentional" << Endl;
            }
        }

        return result;
    }

    TTranslationSrc TGenerativeModel::MakeSrcFromRequest(const TGenerativeRequest& request) const {
        TTranslationSrc translationSrc;
        translationSrc.MaxRepeats = Params.MaxRepeats;
        if (Params.ReturnAllBeamSearchHypothesis) {
            translationSrc.NBest = Params.BeamSize;
        }
        if (auto constraints = ConfigureConstraints(request); !constraints.empty()) {
            translationSrc.Constraint = MakeAtomicShared<NDict::NMT::TMultiConstraint>(std::move(constraints));
        }
        if (Params.UsePrefixedInference) {
            translationSrc.Ids.push_back(Params.DstBosId);
            translationSrc.Words = {u"_BOS_"};
            translationSrc.PrefixIds = request.InputIds;
        } else {
            translationSrc.Ids = request.InputIds;
        }
        translationSrc.DiverseNBest = request.DiverseBeamSearch;
        if (!request.PtuneBlock.Embeddings.empty()) {
            translationSrc.PrefixPtuneBlock = request.PtuneBlock;
        }
        translationSrc.SamplerParams = request.SamplerParams;
        return translationSrc;
    }

    // TGenerativeBoltalka --------------------------------------------------------------------
    TGenerativeBoltalka::TGenerativeBoltalka(const TGenerativeBoltalka::TParams& params)
        : Model(new TGenerativeModel(params.ModelParams, params.TokenizerParams, params.PtunePath))
        , Filter(CreateFilter(params.FilterParams))
        , PreFilter(params.RequestPreFilterParams ? CreateFilter(params.RequestPreFilterParams.GetRef()) : nullptr)
        , PostProcessor(new TPostProcessor(params.PostProcessorParams))
        , PrefixOnlyPtune(params.PrefixOnlyPtune)
    {
        Tokenizer = Model->Tokenizer;
        InitDefaultPtuneEmbeddings(params);
    }

    void TGenerativeBoltalka::InitDefaultPtuneEmbeddings(const TGenerativeBoltalka::TParams& params) {
        if (params.PtunePath.empty()) {
            return;
        }
        const auto file = NDict::NMT::NYNMT::ReadModelProtoWithMeta(TBlob::LockedFromFile(params.PtunePath));
        DefaultPtuneEmbeddings = std::make_shared<TVector<TVector<float>>>(LoadPtuneFromNpz(&file));
    }

    TVector<TGenerativeResponse> TGenerativeBoltalka::GenerateResponses(const TVector<TString>& context,
                                                                        size_t maxResponses,
                                                                        TMaybe<int> seed,
                                                                        bool prefixTokensOnly,
                                                                        const TVector<TString>& spanDelimiters,
                                                                        bool diverseBeamSearch) const {
        return GenerateResponses({
            .Context = context,
            .Seed = seed,
            .NumHypos = maxResponses,
            .PrefixOnly = prefixTokensOnly,
            .SpanDelimiters = spanDelimiters,
            .DiverseBeamSearch = diverseBeamSearch});
    }


    bool TGenerativeBoltalka::CheckRequestHasBadWords(const TGenerativeRequest& request) const {
        if (!PreFilter) {
            return false;
        }

        for (const auto& it : StringSplitter(request.Context.front()).SplitBySet(DELIMETERS)) {
            if (PreFilter->ShouldFilterResponse({TString(it), 0, 0})) {
                Cerr << "Bad word '" << it.Token() << "' in request" << Endl;
                return true;
            }
        }

        return false;
    }

    bool TGenerativeBoltalka::HasDefaultPtuneEmbeddings() const {
        return DefaultPtuneEmbeddings != nullptr;
    }

    TVector<TGenerativeResponse> TGenerativeBoltalka::GenerateResponses(const TGenerativeRequest& request) const {
        const auto result = GenerateResponses(TVector{request});
        Y_ENSURE(!result.empty());
        return result.front();
    }

    TVector<TVector<TGenerativeResponse>> TGenerativeBoltalka::GenerateResponses(const TVector<TGenerativeRequest>& requestsBatch) const {
        TVector<TGenerativeRequest> generativeRequests;
        generativeRequests.reserve(requestsBatch.size());
        Transform(requestsBatch.begin(), requestsBatch.end(), std::back_inserter(generativeRequests),
            [this] (const auto& request) { return PrepareRequest(request); });

        return GeneratePreparedResponses(generativeRequests);
    }

    TVector<TVector<TGenerativeResponse>> TGenerativeBoltalka::GeneratePreparedResponses(const TVector<TGenerativeRequest>& generativeRequests) const {
        auto translations = Model->InferModel(generativeRequests);

        TVector<TVector<TGenerativeResponse>> responsesBatch;
        responsesBatch.reserve(translations.size());
        for (size_t i = 0; i < translations.size(); ++i) {
            auto responses = PrepareResponses(generativeRequests[i].InputIds, translations[i], Model.Get(), Tokenizer.Get(), PostProcessor.Get());
            Filter->FilterResponses(responses);
            responsesBatch.emplace_back(std::move(responses));
        }

        return responsesBatch;
    }


    std::vector<std::vector<float>> TGenerativeBoltalka::GenerateFullEmbed(
        const TVector<TString>& context,
        const TVector<TVector<float>>* ptuneEmbeddings,
        TModelProtoWithMeta* pHead,
        const bool addMask,
        const bool addSep,
        const bool doReverseContext) const
    {
        TVector<int> inputIds;
        const auto* ptune = ptuneEmbeddings ? ptuneEmbeddings : DefaultPtuneEmbeddings.get();
        const size_t ptuneEmbeddingsCount = ptune ? ptune->size() : 0;

        if (doReverseContext) {
            FillInputIdsForEmbeddings(context.rbegin(), context.rend(), inputIds, ptuneEmbeddingsCount, addMask, addSep);
        } else {
            FillInputIdsForEmbeddings(context.begin(), context.end(), inputIds, ptuneEmbeddingsCount, addMask, addSep);
        }

        TMaybe<NDict::NMT::TPtuneBlock> ptuneBlock;
        if (ptune) {
            ptuneBlock.ConstructInPlace();
            FillAndInsertPtuneBlock(inputIds,
                                    ptune,
                                    ptuneBlock.GetRef());
        }
        return Model->Embed(inputIds, ptuneBlock, pHead);
    }

    std::vector<float> TGenerativeBoltalka::GenerateEmbed(
        const TVector<TString>& context,
        const TVector<TVector<float>>* ptuneEmbeddings,
        TModelProtoWithMeta* pHead,
        const bool addMask,
        const bool addSep,
        const bool doReverseContext) const
    {
        return GenerateFullEmbed(
            context,
            ptuneEmbeddings,
            pHead,
            addMask,
            addSep,
            doReverseContext
        ).back();
    }

    std::vector<std::vector<float>> TGenerativeBoltalka::GenerateScores(
        const TVector<TString>& context,
        const TVector<TVector<float>>* ptuneEmbeddings,
        const bool forceEos) const
    {
        TVector<TVector<int>> segmentsIds;
        segmentsIds.reserve(context.size());

        const auto* embeddings = ptuneEmbeddings ? ptuneEmbeddings : DefaultPtuneEmbeddings.get();
        size_t reservedTokensNum = (embeddings ? embeddings->size() : 0) + 1; // + 1 since we do not want to remove "BOS" token
        TVector<int> tmpInputIds;
        for (const auto& segment : context) {
            auto tokenized = Tokenizer->TokenizeLine(segment);

            bool isLastTokenErased = false;
            const auto isOverflowPrevented = Tokenizer->PreventInputOverflow(tmpInputIds, reservedTokensNum, tokenized, isLastTokenErased);

            segmentsIds.push_back(tokenized);
            tmpInputIds.insert(tmpInputIds.end(), tokenized.begin(), tokenized.end());

            if (isOverflowPrevented) {
                break;
            }
        }
        segmentsIds[0].insert(segmentsIds[0].begin(), Tokenizer->Bos());

        TMaybe<NDict::NMT::TPtuneBlock> ptuneBlock;
        if (embeddings) {
            ptuneBlock.ConstructInPlace();
            FillAndInsertPtuneBlock(segmentsIds[0],
                                    embeddings,
                                    ptuneBlock.GetRef());
        }
        if (forceEos) {
            segmentsIds.back().push_back(Tokenizer->Eos());
        }

        TVector<int> inputIds;
        TVector<int> segmentLens;
        segmentLens.push_back(-1);
        for (const auto& segment : segmentsIds) {
            inputIds.insert(inputIds.end(), segment.begin(), segment.end());
            segmentLens.push_back(segment.size() + segmentLens.back());
        }

        auto sampleScores = Model->Score(inputIds, ptuneBlock);

        std::vector<std::vector<float>> result{};
        std::vector<int> forbidden{};
        if (ptuneBlock) {
            forbidden = ptuneBlock->IndicesDesc.second;
        }
        auto curForbidden = forbidden.begin();
        auto curSegmentEnd = segmentLens.begin();
        for (int i = 0; i < (int)sampleScores.size(); i++) {
            if (curSegmentEnd < segmentLens.end() && i >= *curSegmentEnd) {
                result.push_back({});
                curSegmentEnd++;
            }
            if (curForbidden < forbidden.end() && (i + 1 == *curForbidden)) {
                curForbidden++;
                continue;
            }
            result.back().push_back(sampleScores[i]);
        }

        return result;
    }

    std::vector<float> TGenerativeBoltalka::GenerateSingleTokenSuffixesScores(
        const TString& prefix,
        const TVector<TString>& suffixes,
        const TVector<TVector<float>>* ptuneEmbeddings) const
    {
        TVector<int> prefixIds = Tokenizer->TokenizeLine(prefix);

        const auto* embeddings = ptuneEmbeddings ? ptuneEmbeddings : DefaultPtuneEmbeddings.get();

        // + 2 since we do not want to remove "BOS" token and suffix's single-token
        size_t reservedTokensNum = (embeddings ? embeddings->size() : 0) + 2;


        bool isLastTokenErased = false;
        Tokenizer->PreventInputOverflow({}, reservedTokensNum, prefixIds, isLastTokenErased);

        prefixIds.insert(prefixIds.begin(), Tokenizer->Bos());
        TMaybe<NDict::NMT::TPtuneBlock> ptuneBlock;
        if (embeddings) {
            ptuneBlock.ConstructInPlace();
            FillAndInsertPtuneBlock(prefixIds,
                                    embeddings,
                                    ptuneBlock.GetRef());
        }

        TVector<int> suffixesIds;
        for (const auto& suffix : suffixes) {
            TVector<int> singleSuffixIds = Tokenizer->TokenizeLine(suffix);
            Y_ENSURE(singleSuffixIds.size() == 1, "It is allowed to score only suffixes which consist of one token");
            suffixesIds.push_back(singleSuffixIds[0]);
        }

        auto suffixesScores = Model->ScoreSingleTokenSuffixes(prefixIds, suffixesIds, ptuneBlock);

        return suffixesScores;
    }


    TGenerativeRequest TGenerativeBoltalka::PrepareRequest(const TVector<TString>& context) const {
        return PrepareRequest({.Context = context});
    }

    TGenerativeRequest TGenerativeBoltalka::PrepareRequest(const TGenerativeRequest& request) const {
        TGenerativeRequest result(request);

        // reversing since seq2seq wants the order of context to be ascending by time
        std::reverse(result.Context.begin(), result.Context.end());

        auto ptuneEmbeddings = request.PtuneEmbeddings ? request.PtuneEmbeddings : DefaultPtuneEmbeddings.get();

        Tokenizer->PreprocessContext(result.Context);
        result.InputIds = Tokenizer->TokenizeContext(result.Context, ptuneEmbeddings ? ptuneEmbeddings->size() : 0);
        FillAndInsertPtuneBlock(result.InputIds,
                                ptuneEmbeddings,
                                result.PtuneBlock);

        result.SpanDelimitersIds.clear();
        for (const auto& spanDelimiter : request.SpanDelimiters) {
            auto singleSpanDelimiterIds = Tokenizer->TokenizeLine(spanDelimiter);
            result.SpanDelimitersIds.push_back(singleSpanDelimiterIds);
        }

        return result;
    }

    void TGenerativeBoltalka::FillAndInsertPtuneBlock(TVector<int>& inputIds, const TVector<TVector<float>>* ptuneEmbeddings, NDict::NMT::TPtuneBlock& ptuneBlock) const {
        if (inputIds.empty() || ptuneEmbeddings == nullptr) {
            return;
        }
        const auto nPtuneTokens = ptuneEmbeddings->size();
        ptuneBlock.Embeddings.reserve(nPtuneTokens);
        for (const auto& embed : *ptuneEmbeddings) {
            ptuneBlock.Embeddings.emplace_back(embed.begin(), embed.end());
        }

        auto& [ptuneTokenIds, ptuneTokenPositions] = ptuneBlock.IndicesDesc;
        int ptuneBlockSize = PrefixOnlyPtune ? nPtuneTokens : nPtuneTokens / 2;
        std::vector<int> dummySegmentIds(ptuneBlockSize, Tokenizer->Unk());
        for (auto j : xrange(ptuneBlockSize)) {
            ptuneTokenIds.push_back(j);
            ptuneTokenPositions.push_back(j + 1);  // cause of BOS
        }

        inputIds.insert(inputIds.begin() + 1,  // cause of BOS
                        dummySegmentIds.begin(), dummySegmentIds.end());
        if (!PrefixOnlyPtune) {
            for (auto j : xrange(ptuneBlockSize)) {
                ptuneTokenIds.push_back(j + ptuneBlockSize);
                ptuneTokenPositions.push_back(j + inputIds.size());
            }
            inputIds.insert(inputIds.end(),
                            dummySegmentIds.begin(), dummySegmentIds.end());
        }
    }

    template <typename Iterator>
    void TGenerativeBoltalka::FillInputIdsForEmbeddings(Iterator start, Iterator end, TVector<int>& inputIds,
                                                        const size_t ptuneEmbeddingsCount,
                                                        const bool addMask,
                                                        const bool addSep) const {
        // + 1 since we do not want to remove "BOS" token
        size_t reservedTokensNum = ptuneEmbeddingsCount + static_cast<size_t>(addMask) + 1;

        bool popLastSep = addSep;
        for (auto it = start; it != end; ++it) {
            auto tokenized = Tokenizer->TokenizeLine(*it);
            if (addSep) {
                tokenized.push_back(Tokenizer->TokenId("[SEP]"));
            }

            bool isLastTokenErased = false;
            const auto isOverflowPrevented = Tokenizer->PreventInputOverflow(inputIds, reservedTokensNum, tokenized, isLastTokenErased);
            isLastTokenErased &= addSep;

            inputIds.insert(inputIds.end(), tokenized.begin(), tokenized.end());
            if (isOverflowPrevented) {
                break;
            }
        }
        inputIds.insert(inputIds.begin(), Tokenizer->Bos());
        if (popLastSep) {
            inputIds.pop_back();
        }
        if (addMask) {
            inputIds.push_back(Tokenizer->TokenId("[MASK]"));
        }
    }

    const TGenerativeModel* TGenerativeBoltalka::GetModel() const {
        return Model.Get();
    }
} // namespace NGenerativeBoltalka
