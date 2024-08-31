#include "lstm.h"

#include <alice/nlu/libs/embedder/embedding_loading.h>
#include <alice/nlu/libs/tf_nn_model/tensor_helpers.h>

#include <library/cpp/json/json_reader.h>

#include <util/folder/path.h>
#include <util/generic/maybe.h>
#include <util/stream/file.h>
#include <util/system/types.h>

namespace NAlice {
    namespace {
        constexpr TStringBuf MODEL_PROTOBUF_FILE = "model.pb";
        constexpr TStringBuf MODEL_DESCRIPTION_FILE = "model_description.json";
        constexpr TStringBuf SPECIAL_EMBEDDINGS_FILE = "special_embeddings.json";

        const TString SPECIAL_TOKEN_BOS = "[BOS]";
        const TString SPECIAL_TOKEN_SEP = "[SEP]";
        const TString SPECIAL_TOKEN_PAD = "[PAD]";
        const TString SPECIAL_TOKEN_EOS = "[EOS]";
        const TVector<TString> SPECIAL_TOKENS = {SPECIAL_TOKEN_BOS, SPECIAL_TOKEN_SEP, SPECIAL_TOKEN_PAD, SPECIAL_TOKEN_EOS};

        constexpr TStringBuf INPUTS_MAP_KEY = "inputs";
        constexpr TStringBuf OUTPUTS_MAP_KEY = "outputs";
        constexpr TStringBuf NO_ANAPHORA_PROBABILITY_THRESHOLD_MAP_KEY = "no_anaphora_probability_threshold";
        constexpr TStringBuf ANAPHORA_IN_REQUEST_PROBABILITY_THRESHOLD_MAP_KEY = "anaphora_in_request_probability_threshold";
        constexpr TStringBuf ENTITY_PROBABILITY_THRESHOLD_MAP_KEY = "entity_probability_threshold";
        constexpr TStringBuf MAX_HISTORY_LENGTH_MAP_KEY = "max_history_length";

        const TString WORD_EMBEDDINGS_INPUT_KEY = "words";
        const TString SEGMENT_IDS_INPUT_KEY = "segment_ids";
        const TString ENTITY_ROWS_INPUT_KEY = "entity_rows";
        const TString PRONOUN_START_POSITIONS_INPUT_KEY = "pronoun_start_positions";
        const TString PRONOUN_END_POSITIONS_INPUT_KEY = "pronoun_end_positions";
        const TString ENTITY_START_POSITIONS_INPUT_KEY = "entity_start_positions";
        const TString ENTITY_END_POSITIONS_INPUT_KEY = "entity_end_positions";
        const TVector<TString> INPUTS_ORDER = {WORD_EMBEDDINGS_INPUT_KEY, SEGMENT_IDS_INPUT_KEY, ENTITY_ROWS_INPUT_KEY,
                                               PRONOUN_START_POSITIONS_INPUT_KEY, PRONOUN_END_POSITIONS_INPUT_KEY,
                                               ENTITY_START_POSITIONS_INPUT_KEY, ENTITY_END_POSITIONS_INPUT_KEY};

        const TString ENTITY_PROBS_OUTPUT_KEY = "entity_probs";
        const TString PHRASE_PROBS_OUTPUT_KEY = "phrase_probs";
        const TVector<TString> OUTPUTS_ORDER = {ENTITY_PROBS_OUTPUT_KEY, PHRASE_PROBS_OUTPUT_KEY};

        TVector<TString> ReadJsonMappingInOrder(const NJson::TJsonValue& jsonMapping, const TVector<TString>& keysOrder) {
            const auto& mapping = jsonMapping.GetMapSafe();
            Y_ENSURE(keysOrder.size() == mapping.size());

            TVector<TString> values;
            values.reserve(keysOrder.size());
            for (const auto& key : keysOrder) {
                Y_ENSURE(mapping.contains(key));
                values.push_back(mapping.at(key).GetStringSafe());
            }
            return values;
        }

        void AddSegmentPositions(const TVector<size_t>& numTokensPrefSums,
                                 const TMentionInDialogue& position,
                                 TVector<i32>* entityStartPositions,
                                 TVector<i32>* entityEndPositions) {
            Y_ASSERT(entityStartPositions);
            Y_ASSERT(entityEndPositions);

            const size_t numTokensInPreviousSamples = numTokensPrefSums[position.PhrasePos];
            entityStartPositions->push_back(numTokensInPreviousSamples + position.MentionInPhrase.TokenSegment.Begin);
            entityEndPositions->push_back(numTokensInPreviousSamples + position.MentionInPhrase.TokenSegment.End);
        }
    } // namespace anonymous

    TLstmAnaphoraMatcherModel::TLstmAnaphoraMatcherModel(IInputStream* protobufModel,
                                                         const TModelDescription& modelDescription,
                                                         const TSpecialTokenEmbeddings& specialTokenEmbeddings,
                                                         const TTokenEmbedder& tokenEmbedder)
        : TTfNnModel(protobufModel)
        , ModelDescription(modelDescription)
        , SpecialTokenEmbeddings(specialTokenEmbeddings)
        , TokenEmbedder(tokenEmbedder) {

        SetInputNodes(TVector<TString>(ModelDescription.Inputs));
        SetOutputNodes(TVector<TString>(ModelDescription.Outputs));
    }

    TLstmAnaphoraMatcherModel::TLstmAnaphoraMatcherModel(const TString& modelDirName, const TTokenEmbedder& tokenEmbedder)
        : TLstmAnaphoraMatcherModel(MakeHolder<TFileInput>(TFsPath(modelDirName) / MODEL_PROTOBUF_FILE).Get(),
                                    ReadJsonModelDescription(TFsPath(modelDirName) / MODEL_DESCRIPTION_FILE),
                                    ReadJsonSpecialTokenEmbeddings(TFsPath(modelDirName) / SPECIAL_EMBEDDINGS_FILE),
                                    tokenEmbedder)
    {
    }

    TMaybe<TAnaphoraMatch> TLstmAnaphoraMatcherModel::Predict(const TVector<NVins::TSample>& dialogHistory,
                                                              const TVector<TMentionInDialogue>& entityPositions,
                                                              const TMentionInDialogue& pronounPosition,
                                                              const TString& pronounGrammemes) const {
        Y_ASSERT(!dialogHistory.empty());
        if (entityPositions.size() == 0) {
            return Nothing();
        }

        NNeuralNet::TTensorList feed;
        SetFeed(dialogHistory, entityPositions, pronounPosition, &feed);

        auto worker = CreateWorker();
        const auto result = worker->Process(feed);

        // Outputs are collected in accordance with OUTPUTS_ORDER.
        Y_ENSURE(result.size() == 2); // entity probabilities and phrase probabilities
        const auto& entityProbabilities = result[0].tensor<float, 1>();
        const auto& noNeedToResolveProbabilities = result[1].tensor<float, 2>();

        Y_ENSURE(noNeedToResolveProbabilities.dimension(0) == /*batch size*/1);
        Y_ENSURE(noNeedToResolveProbabilities.dimension(1) == /*no anaphora or in-request anaphora*/2);
        if (noNeedToResolveProbabilities(0, 0) > ModelDescription.NoAnaphoraProbabilityThreshold ||
            noNeedToResolveProbabilities(0, 1) > ModelDescription.AnaphoraInRequestProbabilityThreshold) {

            return Nothing();
        }

        Y_ENSURE(entityProbabilities.dimension(0) == entityPositions.ysize());
        size_t mostProbableEntity = 0;
        for (ssize_t entityIdx = 1; entityIdx < entityProbabilities.dimension(0); ++entityIdx) {
            if (entityProbabilities(entityIdx) > entityProbabilities(mostProbableEntity)) {
                mostProbableEntity = entityIdx;
            }
        }
        const double probability = entityProbabilities(mostProbableEntity);
        if (probability < ModelDescription.EntityProbabilityThreshold) {
            return Nothing();
        }

        return TAnaphoraMatch{pronounPosition, entityPositions[mostProbableEntity], probability, pronounGrammemes};
    }

    TLstmAnaphoraMatcherModel::TModelDescription TLstmAnaphoraMatcherModel::ReadJsonModelDescription(IInputStream* input) {
        Y_ASSERT(input);

        NJson::TJsonValue jsonDescription;
        const bool readCorrectly = NJson::ReadJsonTree(input, &jsonDescription);
        Y_ENSURE(readCorrectly);

        const auto& descriptionMap = jsonDescription.GetMapSafe();
        Y_ENSURE(descriptionMap.contains(INPUTS_MAP_KEY));
        Y_ENSURE(descriptionMap.contains(OUTPUTS_MAP_KEY));
        const auto& jsonInputsMap = descriptionMap.at(INPUTS_MAP_KEY);
        const auto& jsonOutputsMap = descriptionMap.at(OUTPUTS_MAP_KEY);

        TModelDescription modelDescription;
        modelDescription.Inputs = ReadJsonMappingInOrder(jsonInputsMap, INPUTS_ORDER);
        modelDescription.Outputs = ReadJsonMappingInOrder(jsonOutputsMap, OUTPUTS_ORDER);

        if (descriptionMap.contains(NO_ANAPHORA_PROBABILITY_THRESHOLD_MAP_KEY)) {
            modelDescription.NoAnaphoraProbabilityThreshold =
                descriptionMap.at(NO_ANAPHORA_PROBABILITY_THRESHOLD_MAP_KEY).GetDoubleSafe();
        }
        if (descriptionMap.contains(ANAPHORA_IN_REQUEST_PROBABILITY_THRESHOLD_MAP_KEY)) {
            modelDescription.AnaphoraInRequestProbabilityThreshold =
                descriptionMap.at(ANAPHORA_IN_REQUEST_PROBABILITY_THRESHOLD_MAP_KEY).GetDoubleSafe();
        }
        if (descriptionMap.contains(ENTITY_PROBABILITY_THRESHOLD_MAP_KEY)) {
            modelDescription.EntityProbabilityThreshold =
                descriptionMap.at(ENTITY_PROBABILITY_THRESHOLD_MAP_KEY).GetDoubleSafe();
        }
        if (descriptionMap.contains(MAX_HISTORY_LENGTH_MAP_KEY)) {
            modelDescription.MaxHistoryLength = descriptionMap.at(MAX_HISTORY_LENGTH_MAP_KEY).GetUIntegerSafe();
        }

        return modelDescription;
    }

    TLstmAnaphoraMatcherModel::TModelDescription TLstmAnaphoraMatcherModel::ReadJsonModelDescription(const TString& filePath) {
        TFileInput modelDescriptionFile(filePath);
        return ReadJsonModelDescription(&modelDescriptionFile);
    }

    TLstmAnaphoraMatcherModel::TSpecialTokenEmbeddings TLstmAnaphoraMatcherModel::ReadJsonSpecialTokenEmbeddings(IInputStream* input) {
        TSpecialTokenEmbeddings specialTokenEmbeddings = LoadEmbeddingsFromJson(input);
        for (const auto& specialToken : SPECIAL_TOKENS) {
            Y_ENSURE(specialTokenEmbeddings.contains(specialToken));
        }

        return specialTokenEmbeddings;
    }

    TLstmAnaphoraMatcherModel::TSpecialTokenEmbeddings TLstmAnaphoraMatcherModel::ReadJsonSpecialTokenEmbeddings(const TString& filePath) {
        TFileInput specialTokenEmbeddingsFile(filePath);
        return ReadJsonSpecialTokenEmbeddings(&specialTokenEmbeddingsFile);
    }

    void TLstmAnaphoraMatcherModel::SetFeed(const TVector<NVins::TSample>& dialogHistory,
                                            const TVector<TMentionInDialogue>& entityPositions,
                                            const TMentionInDialogue& pronounPosition,
                                            NNeuralNet::TTensorList* feed) const {
        Y_ASSERT(feed);
        // Feed is collected in accordance with INPUTS_ORDER.
        SetEmbeddingsAndSegmentIds(dialogHistory, feed);
        SetSegmentPositions(dialogHistory, entityPositions, pronounPosition, feed);
    }

    void TLstmAnaphoraMatcherModel::SetEmbeddingsAndSegmentIds(const TVector<NVins::TSample>& dialogHistory,
                                                               NNeuralNet::TTensorList* feed) const {
        Y_ASSERT(feed);

        TVector<TVector<float>> wordEmbeddings;
        TVector<i32> segmentIds;
        wordEmbeddings.push_back(SpecialTokenEmbeddings.at(SPECIAL_TOKEN_BOS));
        segmentIds.push_back(dialogHistory.size());

        for (size_t phraseIdx = 0; phraseIdx < dialogHistory.size(); ++phraseIdx) {
            if (phraseIdx > 0) {
                wordEmbeddings.push_back(SpecialTokenEmbeddings.at(SPECIAL_TOKEN_SEP));
                segmentIds.push_back(dialogHistory.size() - phraseIdx + 1);
            }

            const auto& phraseTokens = dialogHistory[phraseIdx].Tokens;
            const auto& phraseEmbeddings = TokenEmbedder.EmbedSequence(phraseTokens, SpecialTokenEmbeddings.at(SPECIAL_TOKEN_PAD));

            wordEmbeddings.insert(wordEmbeddings.end(), phraseEmbeddings.begin(), phraseEmbeddings.end());
            segmentIds.insert(segmentIds.end(), phraseEmbeddings.size(), dialogHistory.size() - phraseIdx);
        }

        wordEmbeddings.push_back(SpecialTokenEmbeddings.at(SPECIAL_TOKEN_EOS));
        segmentIds.push_back(/*dialogHistory.size() - dialogHistory.size() + 1*/1);

        feed->push_back(NVins::Convert3DimTableToTensor<float>({wordEmbeddings}));
        feed->push_back(NVins::Convert2DimTableToTensor<i32>({segmentIds}));
    }

    void TLstmAnaphoraMatcherModel::SetSegmentPositions(const TVector<NVins::TSample>& dialogHistory,
                                                        const TVector<TMentionInDialogue>& entityPositions,
                                                        const TMentionInDialogue& pronounPosition,
                                                        NNeuralNet::TTensorList* feed) const {
        Y_ASSERT(feed);

        TVector<i32> entityStartPositions;
        TVector<i32> entityEndPositions;
        TVector<i32> pronounStartPositions;
        TVector<i32> pronounEndPositions;

        TVector<size_t> numTokensPrefSums = {/*[BOS] token*/1};
        numTokensPrefSums.reserve(dialogHistory.size() + 1);
        for (const auto& sample : dialogHistory) {
            numTokensPrefSums.push_back(numTokensPrefSums.back() + sample.Tokens.size() + /*[SEP] or [EOS] token*/1);
        }

        for (const auto& position : entityPositions) {
            AddSegmentPositions(numTokensPrefSums, position, &entityStartPositions, &entityEndPositions);
        }
        AddSegmentPositions(numTokensPrefSums, pronounPosition, &pronounStartPositions, &pronounEndPositions);

        feed->push_back(NVins::MakeZeroTensor<i32>({entityPositions.ysize()})); // entity rows
        feed->push_back(NVins::Convert1DimTableToTensor<i32>(pronounStartPositions));
        feed->push_back(NVins::Convert1DimTableToTensor<i32>(pronounEndPositions));
        feed->push_back(NVins::Convert1DimTableToTensor<i32>(entityStartPositions));
        feed->push_back(NVins::Convert1DimTableToTensor<i32>(entityEndPositions));
    }
} // namespace NAlice
