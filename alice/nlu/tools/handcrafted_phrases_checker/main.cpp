#include <alice/boltalka/libs/text_utils/utterance_transform.h>

#include <kernel/dssm_applier/nn_applier/lib/layers.h>
#include <mapreduce/yt/interface/logging/logger.h>
#include <mapreduce/yt/library/lambda/yt_lambda.h>
#include <mapreduce/yt/util/temp_table.h>

#include <library/cpp/dot_product/dot_product.h>
#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/langs/langs.h>

#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/stream/file.h>

namespace {

constexpr size_t MAX_NUM_TOKENS = 100;
constexpr float DEFAULT_SCORE = -2.f;

const TVector<TString> MODEL_INPUTS = { "reply" };
const TVector<TString> MODEL_OUTPUTS = { "doc_embedding" };

NNlgTextUtils::IUtteranceTransformPtr MakeTransform(ELanguage languageId) {
    return new NNlgTextUtils::TCompoundUtteranceTransform({
        new NNlgTextUtils::TLowerCase(languageId),
        new NNlgTextUtils::TRemovePunctuation(),
        new NNlgTextUtils::TLimitNumTokens(MAX_NUM_TOKENS)
    });
}

TVector<float> GetEmbedding(const NNeuralNetApplier::TModel& model,
                            const NNlgTextUtils::IUtteranceTransformPtr& transform,
                            TStringBuf phrase)
{
    const TVector<TString> modelFeed = { transform->Transform(phrase) };
    const auto sample = MakeAtomicShared<NNeuralNetApplier::TSample>(MODEL_INPUTS, modelFeed);

    TVector<float> embedding;
    model.Apply(sample, MODEL_OUTPUTS, embedding);

    return embedding;
}

TVector<TVector<float>> PrepareEmbeddings(IInputStream* config) {
    NNeuralNetApplier::TModel model;
    model.Load(TBlob::PrechargedFromFile("data/model"));
    model.Init();

    const auto transform = MakeTransform(LANG_TUR);

    TVector<TVector<float>> embeddings;
    TString line;
    while (config->ReadLine(line)) {
        embeddings.emplace_back(GetEmbedding(model, transform, line));
    }

    return embeddings;
}

}

class TMicrointentsYTMapper : public NYT::IMapper<NYT::TNodeReader, NYT::TNodeWriter> {
public:
    Y_SAVELOAD_JOB(TargetEmbeddings, Threshold);

    TMicrointentsYTMapper() = default;

    TMicrointentsYTMapper(const TVector<TVector<float>>& targetEmbeddings, float threshold)
        : TargetEmbeddings(targetEmbeddings)
        , Threshold(threshold)
    {
    }

    void Do(TReader* input, TWriter* output) override {
        for (; input->IsValid(); input->Next()) {
            const auto& inRow = input->GetRow();

            NYT::TNode outRow;
            outRow["utterance_text"] = inRow["reply"];
            outRow["count"] = -inRow["count"].IntCast<int64_t>();

            TVector<float> embedding;
            for (const auto& value : inRow["reply_embedding"].AsList()) {
                embedding.push_back(value.AsDouble());
            }
            const auto score = GetScore(embedding);

            if (score > Threshold) {
                outRow["score"] = score;
                output->AddRow(outRow);
            }
        }
    }

private:
    float GetScore(const TVector<float>& embedding) const {
        float maxScore = DEFAULT_SCORE;
        for (const auto& targetEmbedding : TargetEmbeddings) {
            const float score = DotProduct(targetEmbedding.data(), embedding.data(), embedding.size());
            maxScore = Max(score, maxScore);
        }
        return maxScore;
    }

private:
    TVector<TVector<float>> TargetEmbeddings;
    float Threshold = DEFAULT_SCORE;
};

REGISTER_MAPPER(TMicrointentsYTMapper);

struct TOptions {
    TString Proxy;
    TString InputTablePath;
    TString InputSamplesPath;
    TString OutputTablePath;
    float Threshold = DEFAULT_SCORE;
};

TOptions ReadOptions(int argc, const char** argv) {
    TOptions result;
    TString language;
    auto parser = NLastGetopt::TOpts();

    parser.AddLongOption("proxy")
          .StoreResult(&result.Proxy)
          .DefaultValue("hahn")
          .Help("YT proxy.");

    parser.AddLongOption("input-table")
          .StoreResult(&result.InputTablePath)
          .DefaultValue("//home/voice/dan-anastasev/handcrafted/tr_requests_with_embeddings")
          .Help("Input table absolute path.");

    parser.AddLongOption("input-samples")
          .AddShortName('i')
          .StoreResult(&result.InputSamplesPath)
          .Required()
          .Help("Path to file with samples to check, one sample per line.");

    parser.AddLongOption("output-table")
          .AddShortName('o')
          .StoreResult(&result.OutputTablePath)
          .Required()
          .Help("Output table absolute path.");

    parser.AddLongOption("threshold")
          .StoreResult(&result.Threshold)
          .DefaultValue(DEFAULT_SCORE)
          .Help("Output table absolute path.");

    parser.AddHelpOption();
    parser.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult parserResult{&parser, argc, argv};

    return result;
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    const auto options = ReadOptions(argc, argv);

    TFileInput config(options.InputSamplesPath);

    TVector<TVector<float>> embeddings = PrepareEmbeddings(&config);

    NYT::SetLogger(NYT::CreateStdErrLogger(NYT::ILogger::INFO));
    const auto client = NYT::CreateClient(options.Proxy);

    NYT::TMapOperationSpec spec;
    spec.AddInput<NYT::TNode>(options.InputTablePath)
        .AddOutput<NYT::TNode>(options.OutputTablePath)
        .JobCount(32)
        .Ordered(true);

    client->Map(spec, new TMicrointentsYTMapper(embeddings, options.Threshold));

    NYT::TSortOperationSpec sortSpec;
    sortSpec.AddInput(options.OutputTablePath)
            .Output(options.OutputTablePath)
            .SortBy({"count"});
    client->Sort(sortSpec);

    NYT::TransformCopyIf<NYT::TNode, NYT::TNode>(
        client,
        options.OutputTablePath,
        options.OutputTablePath,
        [](auto& src, auto& dst) {
            dst["utterance_text"] = src["utterance_text"];
            dst["score"] = src["score"];
            dst["count"] = -src["count"].AsInt64();
            return true;
        });

    return 0;
}
