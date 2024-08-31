#pragma once

#include "data.h"
#include "postprocessing.h"
#include "tokenizer.h"

#include <alice/boltalka/extsearch/base/util/memory_mode.h>

#include <dict/mt/libs/nn_base/nn_base.h>
#include <dict/mt/libs/nn/nn.h>
#include <dict/mt/libs/nn/beam_search/constraint.h>
#include <dict/mt/libs/nn/beam_search/repeat_constraint.h>
#include <dict/mt/libs/nn/ynmt/backend.h>
#include <dict/mt/libs/nn/ynmt/encdec/encdec.h>
#include <dict/mt/libs/nn/ynmt/models/transformer_scoring.h>

#include <util/generic/ptr.h>
#include <util/string/builder.h>

namespace NGenerativeBoltalka {

    class TGenerativeModel : public TThrRefBase {
    public:
        using TPtr = TIntrusivePtr<TGenerativeModel>;

        struct TParams {
            TString CheckpointPath;

            size_t NumCpuBackends = 1;
            TVector<int> GpuIds = {};
            size_t NumThreadsPerSession = 1;

            bool ShouldInitializeTranslator = true;
            bool ShouldInitializeScorer = false;
            bool ShouldInitializeEmbedder = false;
            bool ShouldInitializePHead = false;

            size_t BatchSize = 1;
            size_t BeamSize = 1;

            size_t MaxInpLen = 64;
            size_t MaxOutLen = 20;
            size_t MaxPtuneLen = 0;

            TMaybe<size_t> MinOutLen = Nothing(); // value includes _BOS_ and _EOS_ tokens
            int DstEosId = 1;
            int DstBosId = 0;
            bool UsePrefixedInference = false;

            bool ShouldUseGpus = false;

            TString SamplingStrategy = "stochastic_beam_search";
            float SamplingTemperature = 0.6;
            TMaybe<size_t> SamplingTopNLogits = 50;
            float SamplingNucleus = 1.0;

            // time for generation itself
            size_t MaxGenerationMsModel = 1000;

            // time for waiting in queue, batching, etc.
            size_t MaxGenerationMsExecutor = 1000;

            // return all beams instead of the best one
            bool ReturnAllBeamSearchHypothesis = true;

            // include hypotheses that did not finish in time
            bool IncludeLateHypotheses = false;

            // include hypotheses that did not end (generated _EOS_) within maximum output size MaxOutLen
            bool ReturnUnfinishedHypotheses = false;

            // number of token repeats which are allowed, otherwise generation is stopped and the hypo is dropped
            size_t MaxRepeats = 1;

            // tokens that will not be generated during inference
            TVector<TString> BannedTokensToGenerate;

            // same as BannedTokensToGenerate, but provided via file where each line is a token to ban
            TMaybe<TString> FileForBannedTokensToGenerate = Nothing();

            // token ids that will not be generated during inference
            TVector<int> BannedTokenIdsToGenerate;

            // same as BannedTokenIdsToGenerate, but provided via file where each line is a token id to ban
            TMaybe<TString> FileForBannedTokenIdsToGenerate = Nothing();

            // if true then if in BannedTokensToGenerate or in FileForBannedTokensToGenerate there is a token
            // that is UNK for the vocabulary, this will not cause an error. This is to prevent accidental
            // banning of UNK token without intent.
            bool AllowBanUnkTokenToGenerate = false;

            // parameters for EOS boosting
            bool GenerateShorter = false;
            TMaybe<float> BoostEos = Nothing();

            // parameters for Sawtooth boostin
            bool SawtoothBoost = false;
            int SawtoothTokenId = 0;
            float SawtoothBoostMin = -10.0;
            float SawtoothBoostMax = -10.0;
            float SawtoothBoostStep = 1.0;
            bool SawtoothConsumePrefix = false;

            TVector<NDict::NMT::TRepeatConstraint::TNGramPenaltyParams> NGramPenaltyParams;

            EMemoryMode CheckpointMemoryMode = EMemoryMode::Locked;
            NDict::NMT::NYNMT::EHeadMode HeadMode = NDict::NMT::NYNMT::EHeadMode::Scoring;
            TString HeadWeightsName = "";
            bool NormalizeOutTokensRepresentations = true;
            TMaybe<size_t> MaxLinearHeadOutputDim = Nothing();
            bool ReturnFullEmbMatrix = false;
        };

        TGenerativeModel(const TParams& params, const TTokenizer::TParams& tokenizerParams, const TString& pheadPath);
        TVector<NDict::NMT::TTranslation> InferModel(const TGenerativeRequest& request) const;
        TVector<TVector<NDict::NMT::TTranslation>> InferModel(const TVector<TGenerativeRequest>& requestsBatch) const;
        TVector<NDict::NMT::TTranslation> InferModel(const TVector<int>& tokenIds,
                                                     int numHypos,
                                                     TMaybe<int> seed = Nothing(),
                                                     bool prefixTokensOnly = false,
                                                     const TVector<TVector<int>>& spanDelimiters = {},
                                                     bool diverseBeamSearch = false) const;

        std::vector<float> Score(TVector<int> inputIds, const TMaybe<NDict::NMT::TPtuneBlock>& ptuneBlock) const;
        std::vector<std::vector<float>> Embed(TVector<int> inputIds, const TMaybe<NDict::NMT::TPtuneBlock>& ptuneBlock, NDict::NMT::NYNMT::TModelProtoWithMeta* pHead) const;
        std::vector<float> ScoreSingleTokenSuffixes(const TVector<int>& prefixIds,
                                                    const TVector<int>& suffixesIds,
                                                    const TMaybe<NDict::NMT::TPtuneBlock>& ptuneBlock);
        void MaybeErasePrefix(const TVector<int>& inputIds, std::vector<int>* tokenIds) const;
        float CalculateNormalizedScore(const size_t numTokens, const float score) const;
    public:
        TTokenizer::TPtr Tokenizer;
    private:
        std::unique_ptr<NDict::NMT::ITranslator> CreateTranslator(
            const TBlob& modelBlob, NDict::NMT::ISharedNeuralTranslatorManager* translatorManager);
        std::unique_ptr<NDict::NMT::NYNMT::TDecoderLMScorer> CreateScorer(const TBlob& modelBlob, const TString& pheadPath);
        TVector<std::unique_ptr<NDict::NMT::IBeamSearchConstraint>> ConfigureConstraints(const TGenerativeRequest& request) const;
        THashSet<int> PrepareBannedTokenIds(const TVector<TString>& bannedTokensToGenerate,
                                            const TMaybe<TString>& fileForBannedTokensToGenerate,
                                            const TVector<int>& bannedTokenIdsToGenerate,
                                            const TMaybe<TString>& fileForBannedTokenIdsToGenerate,
                                            bool allowUnkBanning) const;
        NDict::NMT::TTranslationSrc MakeSrcFromRequest(const TGenerativeRequest& request) const;

    private:
        TParams Params;
        std::unique_ptr<NDict::NMT::ISharedNeuralTranslatorManager> TranslatorManager;
        std::unique_ptr<NDict::NMT::ITranslator> Translator;
        std::unique_ptr<NDict::NMT::NYNMT::TDecoderLMScorer> Scorer;

        // collected from Params.BannedTokensToGenerate, Params.FileForBannedTokensToGenerate,
        // Params.BannedTokenIdsToGenerate and Params.FileForBannedTokenIdsToGenerate.
        THashSet<int> BannedTokenIdsToGenerate;
    };

    class TGenerativeBoltalka : public TThrRefBase {
    public:
        struct TParams {
            IGenerativeFilter::TParams FilterParams;
            TGenerativeModel::TParams ModelParams;
            TTokenizer::TParams TokenizerParams;
            TPostProcessor::TParams PostProcessorParams;
            TString PtunePath = "";
            TMaybe<IGenerativeFilter::TParams> RequestPreFilterParams;
            bool PrefixOnlyPtune = false;

            bool EmbeddingsAddMask = true;
            bool EmbeddingsAddSep = false;
            bool EmbeddingsDoReverseContext = false;
        };

        using TPtr = TIntrusivePtr<TGenerativeBoltalka>;

    public:
        TGenerativeBoltalka(const TGenerativeBoltalka::TParams& params);

        TVector<TGenerativeResponse> GenerateResponses(const TGenerativeRequest& request) const;
        TVector<TVector<TGenerativeResponse>> GenerateResponses(const TVector<TGenerativeRequest>& requestsBatch) const;
        TVector<TVector<TGenerativeResponse>> GeneratePreparedResponses(const TVector<TGenerativeRequest>& generativeRequests) const;
        /*
            Context is ordered by time in DESCENDING manner: {"i am too thank you", "i am fine, and u?", "how r u?"}
        */
        TVector<TGenerativeResponse> GenerateResponses(const TVector<TString>& context,
                                                       size_t maxResponses,
                                                       TMaybe<int> seed = Nothing(),
                                                       bool prefixTokensOnly = false,
                                                       const TVector<TString>& spanDelimiters = {},
                                                       bool diverseBeamSearch = false) const;
        std::vector<std::vector<float>> GenerateScores(const TVector<TString>& context,
                                                       const TVector<TVector<float>>* ptuneEmbeddings = nullptr,
                                                       const bool forceEos = false) const;
        std::vector<std::vector<float>> GenerateFullEmbed(const TVector<TString>& context,
                                         const TVector<TVector<float>>* ptuneEmbeddings = nullptr,
                                         NDict::NMT::NYNMT::TModelProtoWithMeta* pHead = nullptr,
                                         const bool addMask = false,
                                         const bool addSep = false,
                                         const bool doReverseContext = false) const;
        std::vector<float> GenerateEmbed(const TVector<TString>& context,
                                         const TVector<TVector<float>>* ptuneEmbeddings = nullptr,
                                         NDict::NMT::NYNMT::TModelProtoWithMeta* pHead = nullptr,
                                         const bool addMask = false,
                                         const bool addSep = false,
                                         const bool doReverseContext = false) const;
        std::vector<float> GenerateSingleTokenSuffixesScores(const TString& prefix,
                                                             const TVector<TString>& suffixes,
                                                             const TVector<TVector<float>>* ptuneEmbeddings = nullptr) const;
        TGenerativeRequest PrepareRequest(const TVector<TString>& context) const;
        TGenerativeRequest PrepareRequest(const TGenerativeRequest& request) const;

        bool CheckRequestHasBadWords(const TGenerativeRequest& request) const;
        bool HasDefaultPtuneEmbeddings() const;

    public:
        const TGenerativeModel* GetModel() const;

    private:
        TGenerativeModel::TPtr Model;
        IGenerativeFilter::TPtr Filter;
        IGenerativeFilter::TPtr PreFilter;
        TPostProcessor::TPtr PostProcessor;
        std::shared_ptr<TVector<TVector<float>>> DefaultPtuneEmbeddings;
        bool PrefixOnlyPtune = false;
        TTokenizer::TPtr Tokenizer;

    private:
        void InitDefaultPtuneEmbeddings(const TGenerativeBoltalka::TParams& params);
        void FillAndInsertPtuneBlock(TVector<int>& inputIds,
                                     const TVector<TVector<float>>* ptuneEmbeddings,
                                     NDict::NMT::TPtuneBlock& ptuneBlock) const;
        template <typename Iterator>
        void FillInputIdsForEmbeddings(Iterator start, Iterator end, TVector<int>& inputIds,
                                       const size_t ptuneEmbeddingsCount,
                                       const bool addMask = false,
                                       const bool addSep = false) const;
    };

} // namespace NGenerativeBoltalka
