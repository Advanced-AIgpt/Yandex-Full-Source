#include "dssm_model.h"

#include <cv/imgclassifiers/danet/data_provider/ext/text_data_provider.h>
#include <cv/imgclassifiers/danet/data_provider/ext/encoders/config_factory.h>

#include <cv/imgclassifiers/danet/data_provider/core/readers/memory.h>
#include <cv/imgclassifiers/danet/data_provider/core/generalized_data_provider.h>

#include <util/charset/wide.h>
#include <util/random/shuffle.h>
#include <util/string/split.h>

namespace NNlgServer {

using namespace NNeuralNet;

TDssmModel::TDssmModel(const TFsPath &modelDir, IContextTransformPtr contextTransform, const TString &contextFeaturesLayerName, ui64 numWorkers)
    : Config(new TConfig(modelDir / "init_config.cfg"))
    , ContextTransform(contextTransform)
    , FFTester(new TFeedFwdTester(*Config, numWorkers, EStatisticsOptions::DO_NOT_OUTPUT_STAT))
    , ContextFeaturesLayerName(contextFeaturesLayerName)
    , RepliesWithEmbeddings(NWord2Vec::LoadFromYandex(modelDir / "replies.txt", modelDir / "replies.vec"))
    , AnnSearcher(new TInvMiSearcher("1", modelDir / "replies.imi", /*regionCount*/ 10000))
{
    AnnSearcher->SetModels(nullptr, RepliesWithEmbeddings.Get(), true);
    //AnnSearcher->SetThreadsCount(numWorkers);

    TString readerColumnEncoderTuples;
    Config->GetDataProviderOption(0, "ReaderColumnEncoderTuples", readerColumnEncoderTuples);

    const int numEncoders = 2;
    const int numReplyDataParts = 1;
    const int numReaderColumnEncoderTyples = Count(readerColumnEncoderTuples.data(), readerColumnEncoderTuples.data() + readerColumnEncoderTuples.size(), ';') + 1;
    NN_VERIFY(numReaderColumnEncoderTyples % numEncoders == 0);
    ContextLength = numReaderColumnEncoderTyples / numEncoders - numReplyDataParts;

    auto wordEncoder = CreateEncoderFromConfigFactory(*Config, 0);
    auto trigramEncoder = CreateEncoderFromConfigFactory(*Config, 1);
    TVector<std::pair<TString, IEncoderSPtr>> encoders;
    for (ui64 i = 0; i < ContextLength; ++i) {
        encoders.emplace_back("context" + ToString(i), wordEncoder);
        encoders.emplace_back("context" + ToString(i), trigramEncoder);
    }
    DataPartsEncoder = new TMasterEncoder(encoders);
}

TVector<TUtf16String> TDssmModel::GetReplies(const TUtf16String &context, ui64 /*maxLen*/, double temperature, ui64 numSamples) const {
    TUtf16String query = ContextTransform->Transform(context);
    NN_VERIFY(!query.empty() && query.back() == '\t');
    query = query.substr(0, query.size() - 1);
    Cout << "Generating response for context: " << query << Endl;

    TVector<TString> contextParts = StringSplitter(WideToUTF8(query)).Split('\t');
    NN_VERIFY(contextParts.size() <= ContextLength);
    while (contextParts.size() < ContextLength) {
        contextParts.insert(contextParts.begin(), "");
    }

    TVector<IReader::TColumnNameToDataBlob> contextData(1);
    for (ui64 i = 0; i < ContextLength; ++i) {
        contextData.back().insert({ "context" + ToString(i), TBlob::FromString(contextParts[i]) });
    }
    IReaderSPtr reader = new TMemoryReader(contextData);

    TGeneralizedDataProvider dataProvider(/*batchSize*/ 1, reader, DataPartsEncoder);

    TBatch batch = dataProvider.GetNextBatch();
    NN_VERIFY(batch.size() == 1);

    auto worker = FFTester->GetNetsGraph().GetFreeWorker();
    ui64 workerIdx = worker->GetWorkerIdx();
    //Y_VERIFY(workerIdx < FFTester->GetNeuralNet(0)->GetNumWorkers());
    auto contextFeatures = FFTester->GetLayerOutputsForBatch(batch, ContextFeaturesLayerName, EPassType::PT_VALIDATION, TSample::NO_TIMESTAMPS_FLAG, workerIdx);
    NN_VERIFY(contextFeatures->GetNumSamples() == 1, "Set BatchSize in DataProvider equal to 1!");

    const float* contextFeaturesPtr = contextFeatures->GetSample(0)->AsCpuDense()->GetData();
    size_t contextFeaturesLength = contextFeatures->GetSample(0)->GetNElements();
    NWord2Vec::TVectorType contextFeaturesVector(TVector<float>(contextFeaturesPtr, contextFeaturesPtr + contextFeaturesLength));

    // TODO(alipov): come up with multithreaded solution
    const ui64 numCandidates = Max<ui64>(numSamples, 100);
    TVector<TWordQuality> bestMatches = AnnSearcher->FindBestMatches(&contextFeaturesVector, numCandidates, /*threadNumber*/0);

    TVector<TUtf16String> result;
    result.push_back(ToWtring(bestMatches.front().Word));
    float bestScore = bestMatches.front().Quality;
    for (ui64 i = 1; i < bestMatches.size(); ++i) {
        float score = bestMatches[i].Quality;
        if (bestScore * temperature > score && result.size() >= numSamples) {
            break;
        }
        result.push_back(ToWtring(bestMatches[i].Word));
    }

    Shuffle(result.begin(), result.end());
    result.resize(numSamples);
    return result;
}

}

