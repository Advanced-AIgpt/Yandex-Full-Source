#include <alice/boltalka/generative/service/server/handlers/bert_factor_request_handler.h>
#include <mapreduce/yt/interface/client.h>
#include <library/cpp/getopt/last_getopt.h>
#include <util/string/cast.h>
#include <dict/mt/libs/nn/ynmt/extra/encoder_head.h>


using namespace NYT;
using namespace NGenerativeBoltalka;
using NDict::NMT::NYNMT::TRegressionHead;

TString SafeAsString(const NYT::TNode& node) {
    return node.IsString() ? node.AsString() : "";
}

struct TRow {
    TNode Row;
    NThreading::TFuture<TBertRequestHandler<TRegressionHead>::TResult> Future;
};

using TWriter = TIntrusivePtr<NYT::TTableWriter<NYT::TNode>>;

void ConsumeResults(TVector<TRow>& rows, TWriter writer) {
    for (auto& el : rows) {
        el.Row["bert_score"] = static_cast<double>(el.Future.GetValueSync()[0]);
        writer->AddRow(el.Row);
    }
    rows.clear();
}

int main(int argc, char* argv[]) {
    size_t maxBatchSize;
    size_t maxInputLength;
    size_t contextLen;
    bool truncateAsDialogue;
    TString folder;
    TString inputTable;
    TString outputTable;

    auto opts = NLastGetopt::TOpts::Default();
    opts.AddLongOption("input")
        .StoreResult(&inputTable)
        .Required()
        .Help("Input table");
    opts.AddLongOption("output")
        .StoreResult(&outputTable)
        .Required()
        .Help("Output table");
    opts.AddLongOption("folder")
        .StoreResult(&folder)
        .Required()
        .Help("Model folder");
    opts.AddLongOption("batchsize").StoreResult(&maxBatchSize).DefaultValue(256);
    opts.AddLongOption("maxinputlength").StoreResult(&maxInputLength).DefaultValue(127);
    opts.AddLongOption("contextlen").StoreResult(&contextLen).DefaultValue(3);
    opts.AddLongOption("truncateasdialogue").StoreResult(&truncateAsDialogue).DefaultValue(false);

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    const TBertOutputParams defaultMseParams = {
        1.0, // Scale
        0.0, // Bias
        false, // IsTargetCe
        false, // DoBinarization
        0.0 // BinarizeThreshold
    };

    // TODO : support optional head
    TBertRequestHandler<TRegressionHead> handler(folder, maxBatchSize, maxInputLength, 0, contextLen, truncateAsDialogue, {{0, defaultMseParams}});

    NYT::Initialize(argc, argv);

    auto client = CreateClient("hahn");
    auto reader = client->CreateTableReader<TNode>(inputTable);
    auto writer = client->CreateTableWriter<TNode>(outputTable);

    TVector<TRow> rows;

    for (auto& cursor : *reader) {
        auto& row = cursor.GetRow();
        TVector<TString> sample;
        sample.reserve(contextLen + 1);
        for (size_t i = 0; i < contextLen; ++i) {
            sample.emplace_back(SafeAsString(row["context_" + ToString(contextLen - i - 1)]));
        }
        sample.emplace_back(SafeAsString(row["reply"]));
        rows.push_back({row, handler.ProcessSample(sample)});
        if (rows.size() >= maxBatchSize) {
            ConsumeResults(rows, writer);
        }
    }
    ConsumeResults(rows, writer);

    writer->Finish();

    return 0;
}
