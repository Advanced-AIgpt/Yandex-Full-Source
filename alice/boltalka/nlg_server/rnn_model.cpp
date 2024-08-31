#include "rnn_model.h"

#include <cv/imgclassifiers/danet/data_provider/ext/text_data_provider.h>

#include <util/charset/wide.h>

namespace NNlgServer {

using namespace NNeuralNet;

namespace {

static TUtf16String OneHotToText(const TVector<ui64> &oneHot, TTokenDictSPtr dict) {
    TUtf16String result;
    for (ui64 idx : oneHot) {
        if (!result.empty()) {
            result += ' ';
        }
        TUtf16String token = dict->GetWord(idx);
        result += token;
        if (token == u"_EOS_") {
            break;
        }
    }
    return result;
}

static TUtf16String ReplaceUnknownTokens(TWtringBuf text, TTokenDictSPtr dict) {
    TUtf16String result;
    for (TWtringBuf tok; text.NextTok(' ', tok); ) {
        if (tok.empty()) {
            continue;
        }
        if (!result.empty()) {
            result += ' ';
        }
        if (dict->HasWord(ToWtring(tok))) {
            result += tok;
        } else {
            result += u"_UNK_";
        }
    }
    return result;
}

}

TRnnModel::TRnnModel(const TFsPath &modelDir, IContextTransformPtr contextTransform, ui64 samplingSubNetId, ui64 numWorkers)
    : Config(new TConfig(modelDir / "init_config.cfg"))
    , ContextTransform(contextTransform)
    , RnnTester(new TRnnTester(*Config, numWorkers, EStatisticsOptions::DO_NOT_OUTPUT_STAT))
    , SamplingSubNetId(samplingSubNetId)
{
    TTextEncoderDataProvider dataProvider(new TVectorStringLinesReader({}), *Config, /*dataProviderIdx*/ 0);
    const TWordPerTsEncoder &contextEncoder = dynamic_cast<const TWordPerTsEncoder&>(dataProvider.GetEncoder(0));
    TokenDict = contextEncoder.GetDict();
}

TSampleSPtr TRnnModel::GetSamplingSeed(const TUtf16String &context) const {
    TVector<TUtf16String> query = { ReplaceUnknownTokens(ContextTransform->Transform(context), TokenDict) + u"\t_EOS_" };
    Cout << "Generating response for context: " << TWtringBuf(query.front()).Before('\t') << Endl;

    ILinesReaderSPtr linesReader = new TVectorStringLinesReader(std::move(query));
    TTextEncoderDataProvider dataProvider(linesReader, *Config, /*dataProviderIdx*/ 0);

    TBatch batch = dataProvider.GetNextBatch();
    NN_VERIFY(batch.size() == 1);
    batch[0]->AddInputPart(/*iInput*/ 1, /*nTimestamps*/ 1);
    batch[0]->SetMatrixShallowCopy(/*iInput*/ 1, /*timestamp*/ 0, TCpuSparseCsrMatrix::Zeros(TDims{ 1, TokenDict->GetSize() }));

    return batch[0];
}

TVector<TUtf16String> TRnnModel::GetReplies(const TUtf16String &context, ui64 maxLen, double temperature, ui64 numSamples) const {
    auto seed = GetSamplingSeed(context);
    TVector<ui64> ignore = { TokenDict->GetWordIdx(u"_UNK_") };

    auto worker = RnnTester->GetNetsGraph().GetFreeWorker();
    ui64 workerIdx = worker->GetWorkerIdx();
    TVector<TVector<ui64>> samples = RnnTester->SampleFromNet(seed, maxLen, SamplingSubNetId, numSamples, temperature, workerIdx, ignore);

    TVector<TUtf16String> result;
    for (const auto &sample : samples) {
        result.push_back(OneHotToText(sample, TokenDict));
    }
    return result;
}

}

