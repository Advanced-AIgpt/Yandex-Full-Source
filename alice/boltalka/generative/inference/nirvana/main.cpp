#include <alice/boltalka/generative/inference/core/model.h>
#include <alice/boltalka/libs/text_utils/context_hash.h>
#include <alice/boltalka/libs/text_utils/context_transform.h>

#include <mapreduce/yt/interface/client.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/threading/future/async.h>

#include <util/system/hp_timer.h>
#include <util/folder/path.h>
#include <util/string/join.h>
#include <dict/dictutil/str.h>

using namespace NGenerativeBoltalka;

TVector<TString> GetContexts(const NYT::TNode& row, NNlgTextUtils::TNlgSearchContextTransform& preprocessTransform, size_t contextLen) {
    TVector<TString> contexts;
    contexts.reserve(contextLen);
    for (size_t i = 0; i < contextLen; ++i) {
        contexts.emplace_back(row["context_" + ToString(i)].ConvertTo<TString>());
    }
    return preprocessTransform.Transform(contexts);
}

void PrintResults(const TVector<TString>& contexts, TVector<TGenerativeResponse>& responses) {
    auto str = JoinSeq(" [] ", contexts);
    ReplaceAll(str, "\n", " ");
    Cerr << "INPUT: " << str << Endl;

    if (responses.size() == 0) {
        Cerr << "OUTPUT IS EMPTY" << Endl;
        return;
    }

    for (auto& response : responses) {
        Cerr << "OUTPUT: " << response.Response << " (Score: " << response.Score << ")" << Endl;
    }
}

TVector<TGenerativeResponse> GetResponses(
        const TVector<TString>& contexts, TGenerativeBoltalka& genBoltalka, size_t numHypos, TMaybe<ui64> seed) {
    return genBoltalka.GenerateResponses(contexts, numHypos, seed);
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
            .DefaultValue("model.npz");
    opts.AddLongOption("sampling-mode")
            .Help("Sampling mode for Seq2Seq model")
            .Optional()
            .StoreResult(&generativeBoltalkaParams.ModelParams.SamplingStrategy)
            .DefaultValue("stochastic_beam_search");
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
    opts.AddLongOption("model-min-out-len")
            .Help("Number of tokens to have in the response including _BOS_ and _EOS_")
            .Handler1T<size_t>([&] (size_t value) {
                generativeBoltalkaParams.ModelParams.MinOutLen = value;
            });
    opts.AddLongOption("apply-response-filtering")
            .Help("Whether to apply responses filtering embedded in generative model (in production it is turned on)")
            .Optional()
            .NoArgument()
            .SetFlag(&applyFiltering)
            .DefaultValue(false);
    TString ytProxy;
    opts.AddLongOption("yt-proxy")
            .Help("YT proxy")
            .Optional()
            .StoreResult(&ytProxy)
            .DefaultValue("hahn");
    TString inputYtTable;
    opts.AddLongOption("input-table")
            .Help("Input YT Table to traverse")
            .Required()
            .StoreResult(&inputYtTable);
    size_t contextLen;
    opts.AddLongOption("context-len")
            .Help("Length of the context ('context_0', 'context_1', ...)")
            .StoreResult(&contextLen)
            .DefaultValue(3);
    size_t numLoadThreads;
    opts.AddLongOption("num-load-threads")
            .Help("Number of threads to use to query the translator in parallel")
            .Optional()
            .StoreResult(&numLoadThreads)
            .DefaultValue(1);
    size_t chunkSize;
    opts.AddLongOption("chunk-size")
            .Help("Number of rows to be processed as a single chunk")
            .Optional()
            .StoreResult(&chunkSize)
            .DefaultValue(100000);
    TString outputYtTable;
    opts.AddLongOption("output-table")
            .Help("Output YT Table")
            .Required()
            .StoreResult(&outputYtTable);
    TMaybe<TString> salt = Nothing();
    opts.AddLongOption("salt")
            .Help("Optional salt to be used for each generation (cannot be used with 'salt-column' option, if none of these two options are used, then no seed is used)")
            .Handler1T<TString>([&] (TString value) {
                salt = value;
            });
    TMaybe<TString> saltColumn = Nothing();
    opts.AddLongOption("salt-column")
            .Help("Optional column to retrieve salt from (cannot be used with 'salt' option, if none of these two options are used, then no seed is used)")
            .Handler1T<TString>([&] (TString value) {
                saltColumn = value;
            });
    bool printLog = false;
    opts.AddLongOption("print-log")
            .Help("Whether to print an occasional log messages of amount of data processed")
            .Optional()
            .NoArgument()
            .SetFlag(&printLog);
    NLastGetopt::TOptsParseResult optsParseResult(&opts, argc, argv);

    if (generativeBoltalkaParams.ModelParams.ShouldUseGpus) {
        for (size_t i = 0; i < numBackends; ++i) {
            generativeBoltalkaParams.ModelParams.GpuIds.push_back(i);
        }
    } else {
        generativeBoltalkaParams.ModelParams.NumCpuBackends = numBackends;
    }

    if (!applyFiltering) {
        generativeBoltalkaParams.FilterParams.FilterEmpty = false;
        generativeBoltalkaParams.FilterParams.FilterBadWords = false;
        generativeBoltalkaParams.FilterParams.FilterDuplicateWords = false;
        generativeBoltalkaParams.FilterParams.FilterDuplicateNGrams = false;
        generativeBoltalkaParams.FilterParams.FilterByUniqueWordsRatio = false;
    }

    if (salt.Defined() && saltColumn.Defined()) {
        ythrow yexception() << "Either 'salt' or 'salt-column' can be used (or none of them)";
    }

    generativeBoltalkaParams.TokenizerParams.BpeVocPath = folder / "bpe.voc";
    generativeBoltalkaParams.TokenizerParams.TokenToIdVocPath = folder / "token_to_id.voc";
    generativeBoltalkaParams.ModelParams.CheckpointPath = folder / modelFilename;
    generativeBoltalkaParams.FilterParams.BadWordsDictPath = folder / "bad_dict.txt";

    auto genBoltalka = TGenerativeBoltalka(generativeBoltalkaParams);
    auto preprocessTransform = NNlgTextUtils::TNlgSearchContextTransform(generativeBoltalkaParams.TokenizerParams.Language);


    auto pool = CreateThreadPool(numLoadThreads);

    TVector<NThreading::TFuture<TVector<TGenerativeResponse>>> resultFutures;
    auto client = NYT::CreateClient(ytProxy);
    auto reader = client->CreateTableReader<NYT::TNode>(inputYtTable);
    auto writer = client->CreateTableWriter<NYT::TNode>(outputYtTable);

    int curChunk = 0;
    while (reader->IsValid()) {
        TVector<NYT::TNode> nodesBuffer;
        for (size_t i = 0; i < chunkSize && reader->IsValid(); i++) {
            const auto& row = reader->GetRow();
            nodesBuffer.push_back(row);

            auto context = GetContexts(row, preprocessTransform, contextLen);
            auto seed = (salt.Defined() || saltColumn.Defined()) ?
                    TMaybe<ui64>(NNlgTextUtils::CalculateContextHash(context, salt.Defined() ? salt.GetRef() : saltColumn.GetRef())) : Nothing();

            auto future = NThreading::Async(
                    [context, &genBoltalka, numHypos, seed]() {
                        return GetResponses(context, genBoltalka, numHypos, seed);
                    }, *pool);
            resultFutures.push_back(future);

            reader->Next();
        }

        size_t i = 0;
        for (const auto& f : resultFutures) {
            auto responses = f.GetValueSync();
            for (auto& response : responses) {
                writer->AddRow(NYT::TNode(nodesBuffer[i])("reply", response.Response)("score", response.Score)("context_id", i));
            }

            if (printLog && i == resultFutures.size() - 1) {
                Cerr << "Generated rows: " << curChunk * chunkSize + i + 1 << Endl;
                Cerr << "Current iteration generation: " << Endl;
                PrintResults(GetContexts(nodesBuffer[i], preprocessTransform, contextLen), responses);
            }
            i++;
        }
        curChunk++;
    }
    writer->Finish();
}
