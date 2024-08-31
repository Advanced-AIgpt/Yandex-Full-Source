#include <alice/boltalka/generative/inference/core/model.h>
#include <alice/boltalka/libs/text_utils/context_transform.h>

#include <mapreduce/yt/interface/client.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/threading/future/async.h>

#include <util/system/hp_timer.h>
#include <util/folder/path.h>
#include <util/string/join.h>

using namespace NGenerativeBoltalka;

TVector<TString> GetContexts(const NYT::TNode& row, NNlgTextUtils::TNlgSearchContextTransform& preprocessTransform) {
    TVector<TString> contexts;
    for (const auto& context_key : {"context_0", "context_1", "context_2"}) {
        contexts.push_back(row[context_key].ConvertTo<TString>());
    }
    return preprocessTransform.Transform(contexts);
}

void PrintResults(const TVector<TString>& contexts, TVector<TGenerativeResponse>& responses) {
    Cerr << "INPUT: " << JoinSeq(" [] ", contexts) << Endl;
    for (auto& response : responses) {
        Cerr << "OUTPUT: " << response.Response << " (Score: " << response.Score << ")" << Endl;
    }
}

std::pair<TVector<TGenerativeResponse>, float> GetResponsesWithTiming(
        const TVector<TString>& contexts, TGenerativeBoltalka& genBoltalka, size_t numHypos) {
    THPTimer watch;
    auto responses = genBoltalka.GenerateResponses(contexts, numHypos);
    float timing = watch.Passed();
    return std::make_pair(responses, timing);
}

int main(int argc, const char *argv[]) {
    TGenerativeBoltalka::TParams generativeBoltalkaParams;
    bool applyFiltering;

    NLastGetopt::TOpts opts;
    TFsPath folder;
    opts.AddLongOption("folder")
            .Help("Folder where Seq2Seq checkpoint and vocabularies are stored")
            .Required()
            .StoreResult(&folder);
    TString modelFilename;
    opts.AddLongOption("model-filename")
            .Help("Filename of the checkpoint")
            .Optional()
            .StoreResult(&modelFilename)
            .DefaultValue("release.npz");
    opts.AddLongOption("sampling-mode")
            .Help("Sampling mode for Seq2Seq model")
            .Optional()
            .StoreResult(&generativeBoltalkaParams.ModelParams.SamplingStrategy)
            .DefaultValue("sampling");
    size_t numBackends;
    opts.AddLongOption("num-backends")
            .Help("Number of models (backends)")
            .Optional()
            .RequiredArgument()
            .StoreResult(&numBackends)
            .DefaultValue(1);
    opts.AddLongOption("use-gpus")
            .Help("Whether to use GPUs (if used, num-backends consecutive GPUs are used)")
            .Optional()
            .NoArgument()
            .SetFlag(&generativeBoltalkaParams.ModelParams.ShouldUseGpus);
    opts.AddLongOption("num-threads-per-session")
            .Help("Number of threads for each model (backend)")
            .Optional()
            .StoreResult(&generativeBoltalkaParams.ModelParams.NumThreadsPerSession)
            .DefaultValue(1);
    opts.AddLongOption("batch-size")
            .Help("Batch size")
            .Optional()
            .StoreResult(&generativeBoltalkaParams.ModelParams.BatchSize)
            .DefaultValue(1);
    size_t numHypos;
    opts.AddLongOption("num-hypos")
            .Help("Number of hypothesis to generate by the model")
            .Optional()
            .StoreResult(&numHypos)
            .DefaultValue(1);
    opts.AddLongOption("max-generation-ms")
            .Help("Maximum time for generation (waiting for a future to execute is also included in this time)")
            .Optional()
            .StoreResult(&generativeBoltalkaParams.ModelParams.MaxGenerationMsModel)
            .DefaultValue(1000);
    opts.AddLongOption("beam-size")
            .Help("Beam size")
            .Optional()
            .StoreResult(&generativeBoltalkaParams.ModelParams.BeamSize)
            .DefaultValue(1);
    opts.AddLongOption("model-max-out-len")
            .Help("Number of tokens to truncate hypos by during model running")
            .Optional()
            .StoreResult(&generativeBoltalkaParams.ModelParams.MaxOutLen)
            .DefaultValue(256);
    opts.AddLongOption("apply-response-filtering")
            .Help("Whether to apply responses filtering embedded in generative model (in production it is turned on)")
            .Optional()
            .NoArgument()
            .SetFlag(&applyFiltering)
            .DefaultValue(false);
    TString ytTable;
    opts.AddLongOption("yt-table")
            .Help("YT Table to traverse")
            .Optional()
            .StoreResult(&ytTable)
            .DefaultValue("//home/voice/krom/bucket_14.02.2019/bucket_searchappprod.5000.preprocessed");
    size_t numLoadThreads;
    opts.AddLongOption("num-load-threads")
            .Help("Number of threads to use to query the translator in parallel")
            .Optional()
            .StoreResult(&numLoadThreads)
            .DefaultValue(1);
    bool printResponses = false;
    opts.AddLongOption("print-responses")
            .Help("Whether to print responses while traversing the data")
            .Optional()
            .NoArgument()
            .SetFlag(&printResponses);
    TFsPath outPath;
    opts.AddLongOption("out-path")
            .Help("Out path for analysis")
            .Required()
            .StoreResult(&outPath);
    NLastGetopt::TOptsParseResult optsParseResult(&opts, argc, argv);

    if (generativeBoltalkaParams.ModelParams.ShouldUseGpus) {
        for (size_t i = 0; i < numBackends; ++i) {
            generativeBoltalkaParams.ModelParams.GpuIds.push_back(i);
        }
    } else {
        generativeBoltalkaParams.ModelParams.NumCpuBackends = numBackends;
    }

    if (!applyFiltering) {
        generativeBoltalkaParams.FilterParams.FilterBadWords = false;
        generativeBoltalkaParams.FilterParams.FilterDuplicateWords = false;
    }

    generativeBoltalkaParams.TokenizerParams.BpeVocPath = folder / "full.voc";
    generativeBoltalkaParams.TokenizerParams.TokenToIdVocPath = folder / "token_to_id.voc";
    generativeBoltalkaParams.ModelParams.CheckpointPath = folder / modelFilename;
    generativeBoltalkaParams.FilterParams.BadWordsDictPath = folder / "bad_dict.txt";

    auto genBoltalka = TGenerativeBoltalka(generativeBoltalkaParams);
    auto preprocessTransform = NNlgTextUtils::TNlgSearchContextTransform(generativeBoltalkaParams.TokenizerParams.Language);

    auto client = NYT::CreateClient("hahn");
    auto reader = client->CreateTableReader<NYT::TNode>(ytTable);
    TVector<TVector<TString>> allContexts;

    while (reader->IsValid()) {
        const auto &row = reader->GetRow();
        allContexts.emplace_back(GetContexts(row, preprocessTransform));
        reader->Next();
    }

    auto pool = CreateThreadPool(numLoadThreads);

    THPTimer watch;
    TVector<NThreading::TFuture<std::pair<TVector<TGenerativeResponse>, float>>> resultFutures;
    for (auto &context : allContexts) {
        auto future = NThreading::Async(
                [context, &genBoltalka, numHypos]() {
                    return GetResponsesWithTiming(context, genBoltalka, numHypos);
                }, *pool);
        resultFutures.push_back(future);
    }

    TVector<float> times;
    TVector<int> ids;
    TVector<int> inputs;
    TVector<int> outputs;

    int curId = 0;
    for (const auto& f : resultFutures) {
        auto [responses, time] = f.GetValueSync();
        for (auto &response : responses) {
            ids.push_back(curId);
            inputs.push_back(genBoltalka.PrepareRequest(allContexts[curId]).InputIds.size()); // getting the tokenized input size
            outputs.push_back(response.NumTokens);
            times.push_back(time);
        }
        if (printResponses && responses.size() > 0) {
            PrintResults(allContexts[curId], responses);
        }

        curId++;
    }
    auto experimentTime = watch.Passed();

    float totalTime = 0.0;
    for (float time : times) {
        totalTime += time;
    }

    Y_VERIFY(times.size() == ids.size());
    Y_VERIFY(times.size() == inputs.size());
    Y_VERIFY(times.size() == outputs.size());

    Cerr << "Average time for all: " << totalTime / times.size() << Endl;
    Cerr << "Average generations (not hypothesis) per second: " << resultFutures.size() / experimentTime << Endl;
    Cerr << "Average hypothesis per generation: " << float(times.size()) / allContexts.size() << Endl;

    TFileOutput outFile(outPath);
    for (size_t i = 0; i < times.size(); i++) {
        outFile << times[i] << "\t" << ids[i] << "\t" << inputs[i] << "\t" << outputs[i] << Endl;
    }
    Cerr << "Dumped results in " << outPath << Endl;
}
