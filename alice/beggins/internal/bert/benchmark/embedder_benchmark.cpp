#include <alice/beggins/internal/bert/embedder.h>

#include <quality/relev_tools/bert_models/lib/compress_decompress_node.h>
#include <quality/relev_tools/bert_models/lib/one_file_package.h>

#include <benchmark/benchmark.h>

using namespace NAlice::NBeggins::NInternal;

static void BenchmarkEmbedderProcess(benchmark::State& state) {
    const auto embedder = LoadEmbedder(TBlob::FromFileContent("test_models/model.bert.htxt"),
                                       {.MaxBatchSize = 64, .ExpectedMaxSubBatchesNum = 1, .NumThreads = 4});
    Y_UNUSED(embedder->Process("warmup"));
    constexpr TStringBuf query =
        "привет, я алиса, очень приятно познакомиться, а это тест с длинным запросом пользователя 183";
    for (auto _ : state) {
        benchmark::DoNotOptimize(embedder->Process(query));
    }
}

BENCHMARK(BenchmarkEmbedderProcess);
