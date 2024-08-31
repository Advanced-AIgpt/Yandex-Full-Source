#include "rnn_tagger.h"

#include <alice/nlu/libs/tf_nn_model/tensor_helpers.h>

#include <util/folder/path.h>
#include <util/generic/algorithm.h>
#include <util/generic/map.h>
#include <util/generic/queue.h>
#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/system/types.h>

using namespace NVins;
using namespace NNeuralNet;
using namespace NVins;
using namespace tensorflow;

const TString TRnnTaggerBase::PROTOBUF_MODEL_FILE = "model.pb";
const TString TRnnTaggerBase::MEMMAPPED_MODEL_FILE = "model.mmap";
const TString TRnnTaggerBase::MODEL_DESCRIPTION_FILE = "model_description";

namespace {

constexpr int64_t POSITIVE_CLASS_INDEX = 1;

TString GetTfModelFileName(const TString& modelDirName, EModelFormat modelFormat) {
    if (modelFormat == EModelFormat::Protobuf) {
        return JoinFsPaths(modelDirName, TRnnTaggerBase::PROTOBUF_MODEL_FILE);
    }
    if (modelFormat == EModelFormat::Memmapped) {
        return JoinFsPaths(modelDirName, TRnnTaggerBase::MEMMAPPED_MODEL_FILE);
    }
    Y_ENSURE(false);
}

TString GetModelDescriptionFileName(const TString& modelDirName) {
    return JoinFsPaths(modelDirName, TRnnTaggerBase::MODEL_DESCRIPTION_FILE);
}

struct TModelFileNames {
    TFsPath TfModelFileName;
    TFsPath ModelDescriptionFileName;

    explicit TModelFileNames(const TString& modelDirName, EModelFormat modelFormat = EModelFormat::Memmapped)
        : TfModelFileName(GetTfModelFileName(modelDirName, modelFormat))
        , ModelDescriptionFileName(GetModelDescriptionFileName(modelDirName))
    {
    }

    bool Exists() {
        return TfModelFileName.Exists() && ModelDescriptionFileName.Exists();
    }
};

TVector<TString> RetrieveEncoderInputs(const THashMap<TString, TString>& inputsMapping) {
    TVector<TString> encoderInputs;

    encoderInputs.emplace_back(inputsMapping.at("sequence_lengths"));

    for (size_t inputIdx = 0; inputIdx < 4; ++inputIdx) {
        TString inputName = TString::Join("encoder_input_", ToString(inputIdx));
        auto inputNameIter = inputsMapping.find(inputName);
        if (inputNameIter != inputsMapping.end()) {
            encoderInputs.push_back(inputNameIter->second);
        }
    }
    return encoderInputs;
}

struct TBeamDirection {
    double BeamScore = 0.;
    double TagScore = 0.;
    size_t BeamPrevId = 0;
    i32 TagId = 0;

    TBeamDirection() = default;

    TBeamDirection(double beamScore, double tagScore, size_t beamPrevId, i32 tagId)
        : BeamScore(beamScore)
        , TagScore(tagScore)
        , BeamPrevId(beamPrevId)
        , TagId(tagId)
    {
    }
};

struct TBeamComparator {
    bool operator()(const TBeamDirection& direction1, const TBeamDirection& direction2) {
        return direction1.BeamScore > direction2.BeamScore;
    }
};

struct TBeamSearchParams {
    size_t TopSize = 1;
    size_t BeamWidth = 1;
};

struct TDecoderState {
    Tensor CellState;
    Tensor HiddenState;
    ssize_t Dimension = 0;
};

void DoBeamSearch(
    const Tensor& encoderOutput,
    const NNeuralNet::TTfWorkerPtr& decoder,
    TDecoderState state,
    TBeamSearchParams params,
    TVector<TVector<TBeamDirection>>& beamsWithHistory
) {
    const size_t tokenCount = beamsWithHistory.size() - 1;

    for (size_t wordIdx = 0; wordIdx < tokenCount; ++wordIdx) {
        const auto& beams = beamsWithHistory[wordIdx];

        Tensor prevLabels(GetTfDataType<i32>(), {beams.ysize()});
        auto&& prevLabelsData = prevLabels.vec<i32>();
        for (size_t beamIdx = 0; beamIdx < beams.size(); ++beamIdx) {
            prevLabelsData(beamIdx) = beams[beamIdx].TagId;
        }

        Tensor encoderOutputForWord(
            GetTfDataType<float>(), {static_cast<ssize_t>(encoderOutput.dim_size(2))}
        );
        auto&& encoderOutputForWordData = encoderOutputForWord.tensor<float, 1>();
        auto&& encoderOutputData = encoderOutput.tensor<float, 3>();
        for (ssize_t dataIdx = 0; dataIdx < encoderOutputForWord.dim_size(0); ++dataIdx) {
            encoderOutputForWordData(dataIdx) = encoderOutputData(wordIdx, 0, dataIdx);
        }

        TTensorList decoderFeed({
            prevLabels,
            encoderOutputForWord,
            state.CellState,
            state.HiddenState
        });

        TTensorList decoderOutput = decoder->Process(decoderFeed);
        const auto& predData = decoderOutput[0].tensor<float, 3>();
        const auto& newCellStateData = decoderOutput[1].tensor<float, 2>();
        const auto& newStateData = decoderOutput[2].tensor<float, 2>();

        TPriorityQueue<TBeamDirection, TVector<TBeamDirection>, TBeamComparator> topBeams;

        size_t newBeamsCount = (wordIdx == tokenCount - 1) ? params.TopSize : params.BeamWidth;
        for (size_t beamIdx = 0; beamIdx < beams.size(); ++beamIdx) {
            for (i32 nextTagId = 1; nextTagId < predData.dimensions()[2]; ++nextTagId) {
                topBeams.emplace(
                    beams[beamIdx].BeamScore + predData(0, beamIdx, nextTagId),
                    predData(0, beamIdx, nextTagId),
                    beamIdx,
                    nextTagId
                );
                if (topBeams.size() > newBeamsCount) {
                    topBeams.pop();
                }
            }
        }
        newBeamsCount = Min(newBeamsCount, topBeams.size());

        auto& newBeams = beamsWithHistory[wordIdx + 1];
        newBeams.reserve(newBeamsCount);
        while (!topBeams.empty()) {
            newBeams.push_back(topBeams.top());
            topBeams.pop();
        }
        Reverse(newBeams.begin(), newBeams.end());

        state.CellState = Tensor(GetTfDataType<float>(), {newBeams.ysize(), state.Dimension});
        auto&& decoderCellStateData = state.CellState.tensor<float, 2>();

        state.HiddenState = Tensor(GetTfDataType<float>(), {newBeams.ysize(), state.Dimension});
        auto&& decoderStateData = state.HiddenState.tensor<float, 2>();

        for (size_t beamIdx = 0; beamIdx < newBeams.size(); ++beamIdx) {
            const size_t stateIndex = newBeams[beamIdx].BeamPrevId;
            for (size_t dim = 0; dim < static_cast<size_t>(state.Dimension); ++dim) {
                decoderCellStateData(beamIdx, dim) = newCellStateData(stateIndex, dim);
                decoderStateData(beamIdx, dim) = newStateData(stateIndex, dim);
            }
        }
    }
}

void SetClassPrediction(int64_t classIndex, double classLogProbability, TPrediction& prediction) {
    if (classIndex == POSITIVE_CLASS_INDEX) {
        prediction.IsFromThisClass = true;
        prediction.ClassProbability = std::exp(classLogProbability);
    } else {
        prediction.IsFromThisClass = false;
        prediction.ClassProbability = 1. - std::exp(classLogProbability);
    }
}

TVector<TPrediction> ConvertBeamsToPredictions(
    const TVector<TVector<TBeamDirection>>& beamsWithHistory,
    const TVector<TString>& labelReverseMapping,
    bool hasClassPredictions = false
) {
    TVector<TPrediction> predictions(beamsWithHistory.back().size());
    const size_t tokenCount = beamsWithHistory.size() - 1;
    for (size_t predictionIdx = 0; predictionIdx < predictions.size(); ++predictionIdx) {
        auto& prediction = predictions[predictionIdx];

        prediction.Tags.resize(tokenCount);
        prediction.Probabilities.resize(tokenCount);
        size_t beamId = predictionIdx;

        if (beamsWithHistory.empty()) {
            prediction.FullProbability = 0.;
        } else {
            for (size_t historyPoint = beamsWithHistory.size() - 1; historyPoint > 0; --historyPoint) {
                const auto& beamDirection = beamsWithHistory[historyPoint][beamId];
                prediction.Tags[historyPoint - 1] = labelReverseMapping[beamDirection.TagId];
                prediction.Probabilities[historyPoint - 1] = std::exp(beamDirection.TagScore);
                beamId = beamDirection.BeamPrevId;
            }

            double classLogProbability = 0.;
            if (hasClassPredictions) {
                const auto& classPrediction = beamsWithHistory.front()[beamId];
                classLogProbability = classPrediction.TagScore;
                // -1 to take the padding label into account
                SetClassPrediction(classPrediction.TagId - 1, classLogProbability, prediction);
            }

            const auto tagsLogProbability = beamsWithHistory.back()[predictionIdx].BeamScore - classLogProbability;
            prediction.FullProbability = std::exp(tagsLogProbability);
        }
    }
    return predictions;
}

TPrediction ConvertSimpleTaggerOutputsToPredictions(
    const TTensorList& outputs,
    size_t tokenCount,
    const TVector<TString>& labelReverseMapping
) {
    TPrediction prediction;
    prediction.Tags.reserve(tokenCount);
    prediction.Probabilities.reserve(tokenCount);

    const int64_t classIndex = outputs[2].tensor<int64, 1>()(0);
    const double classLogProbability = outputs[3].tensor<float, 1>()(0);
    SetClassPrediction(classIndex, classLogProbability, prediction);

    double fullLogProbability = 0.;
    auto&& tagIndices = outputs[0].tensor<int64, 2>();
    auto&& tagLogProbabilities = outputs[1].tensor<float, 2>();
    for (size_t wordIdx = 0; wordIdx < tokenCount; ++wordIdx) {
        fullLogProbability += tagLogProbabilities(wordIdx, 0);

        prediction.Tags.push_back(labelReverseMapping[tagIndices(wordIdx)]);
        prediction.Probabilities.push_back(std::exp(tagLogProbabilities(wordIdx)));
    }
    prediction.FullProbability = std::exp(fullLogProbability);

    return prediction;
}

TPrediction GetEmptyClassifyingTaggerPrediction() {
    TPrediction prediction;
    prediction.FullProbability = 1.;
    prediction.ClassProbability = 0.;
    prediction.IsFromThisClass = false;

    return prediction;
}

size_t GetTokenCount(const TSampleFeatures& data) {
    const auto embeddingsIterator = data.DenseSeq.find("alice_requests_emb");
    if (embeddingsIterator != data.DenseSeq.end()) {
        return embeddingsIterator->second.size();
    }
    return data.Sample.Tags.size();
}

} // anonymous

namespace NVins {

void SaveModelDescription(const TString& modelDirName, const TModelDescription& modelDescription) {
    TFileOutput outputFile(GetModelDescriptionFileName(modelDirName));
    modelDescription.Save(&outputFile);
}

bool TRnnTaggerBase::CanBeLoadedFrom(const TString& modelDirName) {
    return TModelFileNames(modelDirName).Exists();
}

TRnnTaggerBase::TRnnTaggerBase(const TString& modelDirName)
    : SessionEstablished(false)
{
    const TModelFileNames fileNames(modelDirName, EModelFormat::Memmapped);
    GraphProcessor.Reset(TTfGraphProcessorBase::New(fileNames.TfModelFileName));

    MemmappedGraph = TBlob::PrechargedFromFile(fileNames.TfModelFileName);

    TFileInput modelDescriptionFile(fileNames.ModelDescriptionFileName);
    ModelDescription.Load(&modelDescriptionFile);

    InitModel();
}

TRnnTaggerBase::TRnnTaggerBase(IInputStream* protobufGraph, IInputStream* modelDescription)
    : SessionEstablished(false)
{
    GraphProcessor.Reset(TTfGraphProcessorBase::New(*protobufGraph));
    ModelDescription.Load(modelDescription);

    InitModel();
}

void TRnnTaggerBase::InitModel() {
    InitLabelReverseMapping();
    InitSparseFeatureSizes();
}

void TRnnTaggerBase::InitLabelReverseMapping() {
    const auto& labelMapping = ModelDescription.FeaturesMapping.at("label");

    LabelReverseMapping.reserve(labelMapping.size() + 1);
    LabelReverseMapping.push_back("EMPTY");
    for (const auto& mappingElement : labelMapping) {
        LabelReverseMapping.push_back(mappingElement.first);
    }

    Sort(
        LabelReverseMapping.begin() + 1,
        LabelReverseMapping.end(),
        [&](const TString& label1, const TString& label2) {
            return labelMapping.at(label1) < labelMapping.at(label2);
        }
    );
}

void TRnnTaggerBase::InitSparseFeatureSizes() {
    const auto& featuresMapping = ModelDescription.FeaturesMapping;
    for (auto&& featureToMapping : featuresMapping) {
        size_t featureSize = 0;
        for (const auto& mappingElement : featureToMapping.second) {
            featureSize = Max(featureSize, mappingElement.second);
        }
        SparseFeatureSizes[featureToMapping.first] = featureSize + 1; // + 1 for the zero-padding index
    }
}

bool TRnnTaggerBase::UsesSparseFeature(const TString& featureName) const {
    const auto& usedFeatures = ModelDescription.UsedSparseFeatures;
    return Find(usedFeatures.begin(), usedFeatures.end(), featureName) != usedFeatures.end();
}

bool TRnnTaggerBase::UsesDenseFeature(const TString& featureName) const {
    const auto& usedFeatures = ModelDescription.UsedDenseFeatures;
    return Find(usedFeatures.begin(), usedFeatures.end(), featureName) != usedFeatures.end();
}

void TRnnTaggerBase::Save(const TString& modelDirName) const {
    Y_ENSURE(MemmappedGraph.Defined(), "Save is possible only with models loaded from memmapped format");

    const TModelFileNames fileNames(modelDirName);

    TFileOutput modelDescriptionFile(fileNames.ModelDescriptionFileName);
    ModelDescription.Save(&modelDescriptionFile);

    TFileOutput modelFile(fileNames.TfModelFileName);
    modelFile.Write(MemmappedGraph->Data(), MemmappedGraph->Size());
}

bool TRnnTaggerBase::IsSessionEstablished() const {
    return SessionEstablished;
}

void TRnnTaggerBase::EstablishSession(const TSessionConfig& config) {
    auto sessionOptions = GraphProcessor->BuildCommonSessionOptions(
        config.NumInterOpThreads,
        config.NumIntraOpThreads,
        TTfSession::CUDA_NO_DEVICE
    );

    sessionOptions.config.set_use_per_session_threads(config.UsePerSessionThreads);

    Session.Reset(GraphProcessor->MakeSession(sessionOptions));

    SessionEstablished = true;
}

TTensorList TRnnTaggerBase::ConvertDataToEncoderFeed(const TSampleFeatures& data, ssize_t tokenCount) const {
    TTensorList encoderFeed;

    Tensor sequenceLengths = Tensor(GetTfDataType<i32>(), {1});
    sequenceLengths.vec<i32>()(0) = tokenCount;
    encoderFeed.push_back(sequenceLengths);

    Tensor wordEmbeddings = ConvertWordDataToEncoderFeed(data, tokenCount);
    encoderFeed.push_back(wordEmbeddings);

    for (auto&& featureName : ModelDescription.UsedSparseFeatures) {
        if (featureName == "postag") {
            Tensor postag = ConvertPostagDataToEncoderFeed(data, tokenCount);
            encoderFeed.push_back(postag);
        } else {
            Tensor ner = ConvertNerDataToEncoderFeed(data, tokenCount, featureName);
            encoderFeed.push_back(ner);
        }
    }

    return encoderFeed;
}

Tensor TRnnTaggerBase::ConvertPostagDataToEncoderFeed(const TSampleFeatures& data, ssize_t tokenCount) const {
    if (!data.SparseSeq.contains("postag")) {
        return MakeZeroTensor<i32>({tokenCount, 1});
    }

    const auto& postag = data.SparseSeq.at("postag");
    const auto& postagMapping = ModelDescription.FeaturesMapping.at("postag");
    Tensor result = Tensor(GetTfDataType<i32>(), {tokenCount, 1});
    auto&& resultData = result.tensor<i32, 2>();
    for (ssize_t tagIdx = 0; tagIdx < tokenCount; ++tagIdx) {
        if (postag[tagIdx].empty()) {
            resultData(tagIdx, 0) = 0;
        } else {
            const TString& tag = postag[tagIdx][0].Value;
            resultData(tagIdx, 0) = postagMapping.contains(tag) ? postagMapping.at(tag) : 0;
        }
    }
    return result;
}

Tensor TRnnTaggerBase::ConvertWordDataToEncoderFeed(const TSampleFeatures& data, ssize_t tokenCount) const {
    const auto& embeddings = data.DenseSeq.at("alice_requests_emb");
    Tensor result(GetTfDataType<float>(), {tokenCount, 1, embeddings[0].ysize()});
    auto&& resultData = result.tensor<float, 3>();
    for (ssize_t dim0Iter = 0; dim0Iter < tokenCount; ++dim0Iter) {
        for (size_t dim1Iter = 0; dim1Iter < embeddings[dim0Iter].size(); ++dim1Iter) {
            resultData(dim0Iter, 0, dim1Iter) = embeddings[dim0Iter][dim1Iter];
        }
    }
    return result;
}

Tensor TRnnTaggerBase::ConvertNerDataToEncoderFeed(
    const TSampleFeatures& data,
    ssize_t tokenCount,
    const TString& featureName
) const {
    const auto& featuresMapping = ModelDescription.FeaturesMapping;

    ssize_t nerCount = SparseFeatureSizes.at(featureName);
    if (!data.SparseSeq.contains(featureName)) {
        return MakeZeroTensor<float>({tokenCount, 1, nerCount});
    }

    const auto& sparseSeq = data.SparseSeq.at(featureName);
    Tensor result = MakeZeroTensor<float>({sparseSeq.ysize(), 1, nerCount});
    auto&& resultData = result.tensor<float, 3>();
    for (size_t featuresSetIdx = 0; featuresSetIdx < sparseSeq.size(); ++featuresSetIdx) {
        const auto& featuresSet = sparseSeq[featuresSetIdx];
        for (const TSparseFeature& feature : featuresSet) {
            const auto& nerMapping = featuresMapping.at(featureName);
            size_t position = nerMapping.contains(feature.Value) ? nerMapping.at(feature.Value) : 0;
            resultData(featuresSetIdx, 0, position) = feature.Weight;
        }
    }
    return result;
}

TRnnTagger::TRnnTagger(const TString& modelDirName)
    : TRnnTaggerBase(modelDirName)
{
    InitSessionNodes({"encoder_out"});
}

TRnnTagger::TRnnTagger(IInputStream* protobufGraph, IInputStream* modelDescription)
    : TRnnTaggerBase(protobufGraph, modelDescription)
{
    InitSessionNodes({"encoder_out"});
}

void TRnnTagger::InitSessionNodes(const TVector<TString>& encoderOutputs) {
    const auto& inputsMapping = GetModelDescription().InputsMapping;
    const auto& outputsMapping = GetModelDescription().OutputsMapping;

    EncoderInputs = RetrieveEncoderInputs(inputsMapping);

    EncoderOutputs.clear();
    for (const auto& encoderOutputName : encoderOutputs) {
        EncoderOutputs.push_back(outputsMapping.at(encoderOutputName));
    }

    DecoderInputs = {
        inputsMapping.at("prev_labels"),
        inputsMapping.at("encoder_res"),
        inputsMapping.at("decoder_cell_state"),
        inputsMapping.at("decoder_state")
    };
    DecoderOutputs = {
        outputsMapping.at("preds"),
        outputsMapping.at("decoder_cell_state"),
        outputsMapping.at("decoder_state")
    };
}

TVector<TVector<TPrediction>> TRnnTagger::PredictTopForBatch(
    const TVector<TSampleFeatures>& data,
    size_t topSize,
    size_t beamWidth
) const {
    Y_ENSURE(IsSessionEstablished());

    TVector<TVector<TPrediction>> result;
    result.reserve(data.size());
    for (const TSampleFeatures& sampleFeatures : data) {
        result.push_back(PredictTop(sampleFeatures, topSize, beamWidth));
    }
    return result;
}

TVector<TPrediction> TRnnTagger::PredictTop(
    const TSampleFeatures& data,
    size_t topSize,
    size_t beamWidth
) const {
    Y_ENSURE(IsSessionEstablished());

    const size_t tokenCount = GetTokenCount(data);
    const i32 emptyLabelId = GetModelDescription().NumLabels;

    TVector<TVector<TBeamDirection>> beamsWithHistory(tokenCount + 1);
    beamsWithHistory[0].emplace_back(/*beamScore*/0.0, /*tagScore*/0.0, /*beamPrevId*/0, emptyLabelId);

    if (tokenCount == 0) {
        return ConvertBeamsToPredictions(beamsWithHistory, GetLabelReverseMapping());
    }

    const TTensorList encoderFeed = ConvertDataToEncoderFeed(data, tokenCount);
    const Tensor encoderOutput = CreateEncoder()->Process(encoderFeed)[0];

    TDecoderState state;
    state.Dimension = static_cast<ssize_t>(GetModelDescription().DecoderDimension);
    state.CellState = MakeZeroTensor<float>({1, state.Dimension});
    state.HiddenState = MakeZeroTensor<float>({1, state.Dimension});

    auto decoder = CreateDecoder();
    TBeamSearchParams params{topSize, beamWidth};
    DoBeamSearch(encoderOutput, decoder, state, params, beamsWithHistory);

    return ConvertBeamsToPredictions(beamsWithHistory, GetLabelReverseMapping());
}

NNeuralNet::TTfWorkerPtr TRnnTagger::CreateEncoder() const {
    Y_ENSURE(!EncoderInputs.empty());
    Y_ENSURE(!EncoderOutputs.empty());

    return GetSession()->MakeWorker(EncoderInputs, EncoderOutputs);
}

NNeuralNet::TTfWorkerPtr TRnnTagger::CreateDecoder() const {
    Y_ENSURE(!DecoderInputs.empty());
    Y_ENSURE(!DecoderOutputs.empty());

    return GetSession()->MakeWorker(DecoderInputs, DecoderOutputs);
}

TClassifyingRnnTagger::TClassifyingRnnTagger(const TString& modelDirName)
    : TRnnTagger(modelDirName)
{
    InitSessionNodes({"encoder_out", "class_preds"});
}

TClassifyingRnnTagger::TClassifyingRnnTagger(IInputStream* protobufGraph, IInputStream* modelDescription)
    : TRnnTagger(protobufGraph, modelDescription)
{
    InitSessionNodes({"encoder_out", "class_preds"});
}

TVector<TPrediction> TClassifyingRnnTagger::PredictTop(
    const TSampleFeatures& data,
    size_t topSize,
    size_t beamWidth
) const {
    Y_ENSURE(IsSessionEstablished());

    const size_t tokenCount = GetTokenCount(data);
    if (tokenCount == 0) {
        return {GetEmptyClassifyingTaggerPrediction()};
    }

    const TTensorList encoderFeed = ConvertDataToEncoderFeed(data, tokenCount);
    const TTensorList encoderOutputs = CreateEncoder()->Process(encoderFeed);
    const Tensor encoderOutput = encoderOutputs[0];

    TVector<TVector<TBeamDirection>> beamsWithHistory(tokenCount + 1);

    const auto classPredictionTensor = encoderOutputs[1].tensor<float, 2>();
    for (size_t classIndex = 0; classIndex <= 1; ++classIndex) {
        beamsWithHistory[0].emplace_back(
            /*beamScore*/classPredictionTensor(0, classIndex),
            /*tagScore*/classPredictionTensor(0, classIndex),
            /*beamPrevId*/0,
            /*TagId*/classIndex + 1  // +1 for the padding label
        );
    }

    TDecoderState state;
    state.Dimension = static_cast<ssize_t>(GetModelDescription().DecoderDimension);
    state.CellState = MakeZeroTensor<float>({2, state.Dimension});
    state.HiddenState = MakeZeroTensor<float>({2, state.Dimension});

    auto decoder = CreateDecoder();
    TBeamSearchParams params{topSize, beamWidth};
    DoBeamSearch(encoderOutput, decoder, state, params, beamsWithHistory);

    return ConvertBeamsToPredictions(beamsWithHistory, GetLabelReverseMapping(), /*hasClassPredictions*/true);
}

TClassifyingSimpleRnnTagger::TClassifyingSimpleRnnTagger(const TString& modelDirName)
    : TRnnTaggerBase(modelDirName)
{
    InitSessionNodes();
}

TClassifyingSimpleRnnTagger::TClassifyingSimpleRnnTagger(IInputStream* protobufGraph, IInputStream* modelDescription)
    : TRnnTaggerBase(protobufGraph, modelDescription)
{
    InitSessionNodes();
}

void TClassifyingSimpleRnnTagger::InitSessionNodes() {
    const auto& inputsMapping = GetModelDescription().InputsMapping;
    const auto& outputsMapping = GetModelDescription().OutputsMapping;

    Inputs = RetrieveEncoderInputs(inputsMapping);
    Outputs = {
        outputsMapping.at("tag_preds"),
        outputsMapping.at("tag_pred_logprobs"),
        outputsMapping.at("class_preds"),
        outputsMapping.at("class_pred_logprobs"),
    };
}

TPrediction TClassifyingSimpleRnnTagger::Predict(const TSampleFeatures& data) const {
    Y_ENSURE(IsSessionEstablished());

    const size_t tokenCount = GetTokenCount(data);
    if (tokenCount == 0) {
        return GetEmptyClassifyingTaggerPrediction();
    }

    const TTensorList feed = ConvertDataToEncoderFeed(data, tokenCount);
    const TTensorList outputs = CreateWorker()->Process(feed);

    return ConvertSimpleTaggerOutputsToPredictions(outputs, tokenCount, GetLabelReverseMapping());
}

NNeuralNet::TTfWorkerPtr TClassifyingSimpleRnnTagger::CreateWorker() const {
    Y_ENSURE(!Inputs.empty());
    Y_ENSURE(!Outputs.empty());

    return GetSession()->MakeWorker(Inputs, Outputs);
}

} // NVins
