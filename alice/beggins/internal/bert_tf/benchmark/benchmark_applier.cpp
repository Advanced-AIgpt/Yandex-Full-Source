#include <alice/beggins/internal/bert_tf/applier.h>

#include <contrib/libs/benchmark/include/benchmark/benchmark.h>

#include <library/cpp/testing/common/env.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

using namespace NAlice::NBeggins::NBertTfApplier;

TVector<TUtf32String> words = {U"раз", U"два", U"три"};

auto MakeInput(size_t len) {
    auto result = words[0];
    for (size_t i = 1; i < len; i++) {
        result.append(U" ");
        result.append(words[i % 3]);
    }
    return result;
}

const TString dataPath = BuildRoot() + "/alice/beggins/internal/bert_tf/ut/data/";
const TString startTriePath = dataPath + "start.trie";
const TString continuationTriePath = dataPath + "cont.trie";
const TString vocabFilePath = dataPath + "vocab.txt";
const TString graphDefPath = dataPath + "model.pb";

std::unique_ptr<TApplier> applier;

void initApplier() {
    static bool wasInited = false;
    if (wasInited)
        return;
    applier = std::make_unique<TApplier>(
        std::make_unique<TTokenizer>(TTrie::FromFile(startTriePath), TTrie::FromFile(continuationTriePath)),
        TBertDict::FromFile(vocabFilePath),
        TApplier::TTfGraphConfig{.GraphDefPath = graphDefPath,
                                 .SequenceLength = 40,
                                 .TokenEmbeddingsOutput = "bert/bert_encoder/encoder/Reshape_9",
                                 .SentenceEmbeddingOutput = "output_embedding"},
        TApplier::TTfSessionConfig{.NumInterOpThreads = 4, .NumIntraOpThreads = 4});
    wasInited = true;
    auto warmup_result = applier->Apply({MakeInput(10)});
    Y_UNUSED(warmup_result);
}

void BM_Applier(benchmark::State& state) {
    initApplier();
    auto input = MakeInput(state.range());
    for (auto _ : state) {
        applier->Apply({input});
    }
}

BENCHMARK(BM_Applier)->DenseRange(1, 38, 1);
