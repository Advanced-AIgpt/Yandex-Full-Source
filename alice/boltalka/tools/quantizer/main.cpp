#include "quantizer.h"
#include "borders_retrieval.h"
#include "metrics.h"

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/operation.h>
#include <mapreduce/yt/util/temp_table.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>

#include <util/generic/vector.h>
#include <util/generic/ymath.h>
#include <util/string/vector.h>
#include <util/string/split.h>
#include <util/string/join.h>
#include <util/string/cast.h>

class TApplyQuantization : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TApplyQuantization() = default;

    TApplyQuantization(const THashMap<TString, TQuantizer>& column2Quantizer)
        : Column2Quantizer(column2Quantizer)
    {
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        const size_t embeddingDim = input->GetRow()[Column2Quantizer.begin()->first].AsString().length() / sizeof(float);
        for (; input->IsValid(); input->Next()) {
            auto row = input->GetRow();
            for (const auto& p : Column2Quantizer) {
                const auto& column = p.first;
                const auto& quantizer = p.second;
                const auto* embeddingPtr = reinterpret_cast<const float*>(row[column].AsString().data());
                TVector<float> values(embeddingPtr, embeddingPtr + embeddingDim);
                auto valuesQuant = quantizer.Apply(values);
                row[column] = TString(reinterpret_cast<const char*>(valuesQuant.data()), sizeof(i8) * valuesQuant.size());
            }
            output->AddRow(row);
        }
    }

    Y_SAVELOAD_JOB(Column2Quantizer);

private:
    THashMap<TString, TQuantizer> Column2Quantizer;
};
REGISTER_MAPPER(TApplyQuantization);

int Apply(int argc, const char* argv[]) {
    TString serverProxy;
    TString inputTable;
    TString outputTable;
    TString columnsString;
    TString quantilesString;
    TString bordersTable;
    bool reuseBorders = false;

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption();
    opts
        .AddLongOption('p', "proxy")
        .DefaultValue("hahn")
        .StoreResult(&serverProxy)
        .Help("YT server.\n\n\n");
    opts
        .AddLongOption('i', "input")
        .RequiredArgument("TABLE")
        .Required()
        .StoreResult(&inputTable);
    opts
        .AddLongOption('o', "output")
        .RequiredArgument("TABLE")
        .Required()
        .StoreResult(&outputTable);
    opts
        .AddLongOption('c', "columns")
        .DefaultValue("context_embedding,reply_embedding")
        .StoreResult(&columnsString);
    opts
        .AddLongOption('q', "quantiles")
        .Required()
        .StoreResult(&quantilesString);
    opts
        .AddLongOption('b', "borders")
        .RequiredArgument("TABLE")
        .StoreResult(&bordersTable);
    opts
        .AddLongOption("reuse-borders")
        .NoArgument()
        .Handler0([&]() {
            reuseBorders = true;
        });

    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');
    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    const size_t numBits = 8;
    const int numBins = 1 << numBits;
    const float outputMax = numBins / 2. - 1.;
    const float outputMin = -numBins / 2.;

    TVector<TString> columns = StringSplitter(columnsString).Split(',');
    TVector<float> quantiles;
    for (const auto& v: StringSplitter(quantilesString).Split(',')) {
        quantiles.push_back(FromString<float>(v.Token()));
    }
    Y_VERIFY(columns.size() == quantiles.size());
    Y_VERIFY(*MaxElement(quantiles.begin(), quantiles.end()) < 0.01);

    TColumnToQuantiles column2Quantiles;
    for (size_t i = 0; i < columns.size(); ++i) {
        column2Quantiles[columns[i]].push_back(quantiles[i]);
    }

    auto client = NYT::CreateClient(serverProxy);
    auto tmpTable = NYT::TTempTable(client);
    if (!reuseBorders) {
        if (bordersTable == "") {
            bordersTable = tmpTable.Name();
        }
        CalcEmbeddingsBorders(client, inputTable, &column2Quantiles, bordersTable);
    }
    auto column2Borders = ReadEmbeddingsBorders(client, &column2Quantiles, bordersTable);

    THashMap<TString, TQuantizer> column2Quantizer;
    for (auto column : columns) {
        Y_VERIFY(column2Borders[column].size() == 1);
        auto q = column2Quantiles[column][0];
        const auto& borders = column2Borders[column][0];
        column2Quantizer[column] = TQuantizer(borders.first, borders.second, outputMin, outputMax, q);
    }

    client->Map(
        NYT::TMapOperationSpec()
            .AddInput<NYT::TNode>(inputTable)
            .AddOutput<NYT::TNode>(outputTable),
        new TApplyQuantization(column2Quantizer));

    return 0;
}

int TuneQuantiles(int argc, const char* argv[]) {
    TString serverProxy;
    TString inputTable;
    TString outputTable;
    float quantileLow;
    float quantileHigh;
    float quantileStep;
    TString queryTable;
    float contextWeight;
    TString topSizesString;
    size_t threadCount;
    TString replyColumnsString;
    TString bordersTable;
    bool reuseBorders = false;

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption();
    opts
        .AddLongOption('p', "proxy")
        .DefaultValue("hahn")
        .StoreResult(&serverProxy)
        .Help("YT server.\n\n\n");
    opts
        .AddLongOption('i', "input")
        .RequiredArgument("TABLE")
        .Required()
        .StoreResult(&inputTable);
    opts
        .AddLongOption('o', "output")
        .RequiredArgument("TABLE")
        .Required()
        .StoreResult(&outputTable);
    opts
        .AddLongOption("low")
        .RequiredArgument("FLOAT")
        .DefaultValue(1e-7)
        .StoreResult(&quantileLow);
    opts
        .AddLongOption("high")
        .RequiredArgument("FLOAT")
        .DefaultValue(2e-3)
        .StoreResult(&quantileHigh);
    opts
        .AddLongOption("step")
        .RequiredArgument("FLOAT")
        .DefaultValue(2.)
        .StoreResult(&quantileStep);
    opts
        .AddLongOption('q', "query")
        .RequiredArgument("TABLE")
        .StoreResult(&queryTable);
    opts
        .AddLongOption("context-weight")
        .RequiredArgument("FLOAT")
        .DefaultValue(0.5)
        .StoreResult(&contextWeight);
    opts
        .AddLongOption("top-sizes-for-metrics")
        .DefaultValue("100,50")
        .StoreResult(&topSizesString);
    opts
        .AddLongOption('T', "thread-count")
        .RequiredArgument("INT")
        .Optional()
        .DefaultValue(8)
        .StoreResult(&threadCount);
    opts
        .AddLongOption("reply-columns")
        .DefaultValue("reply,context_0,context_1,context_2")
        .StoreResult(&replyColumnsString)
        .Help("Reply columns taken into account in ranking metrics calculation");
    opts
        .AddLongOption('b', "borders")
        .RequiredArgument("TABLE")
        .StoreResult(&bordersTable);
    opts
        .AddLongOption("reuse-borders")
        .NoArgument()
        .Handler0([&]() {
            reuseBorders = true;
        });

    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');
    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    const size_t numBits = 8;
    const int numBins = 1 << numBits;
    const float outputMax = numBins / 2. - 1.;
    const float outputMin = -numBins / 2.;

    TVector<float> quantileCandidates = {0.};
    for (float q = quantileLow; q < quantileHigh; q *= quantileStep) {
        quantileCandidates.push_back(q);
    }
    Y_VERIFY(*MaxElement(quantileCandidates.begin(), quantileCandidates.end()) < 0.01);

    TVector<size_t> topSizes;
    for (const auto &v : StringSplitter(topSizesString).Split(',')) {
        topSizes.push_back(FromString<int>(v.Token()));
    }

    const TVector<TString> columns = {"context_embedding", "reply_embedding"};

    TColumnToQuantiles column2Quantiles;
    for (auto column : columns) {
        column2Quantiles.emplace(column, quantileCandidates);
    }

    TVector<TString> replyColumns = StringSplitter(replyColumnsString).Split(',');

    auto client = NYT::CreateClient(serverProxy);
    auto tmpTable = NYT::TTempTable(client);
    if (!reuseBorders) {
        if (bordersTable == "") {
            bordersTable = tmpTable.Name();
        }
        CalcEmbeddingsBorders(client, inputTable, &column2Quantiles, bordersTable);
    }
    auto column2Borders = ReadEmbeddingsBorders(client, &column2Quantiles, bordersTable);

    THashMap<TString, TVector<TQuantizer>> column2Quantizers;
    for (auto column : columns) {
        Cerr << column << "\n";
        for (size_t i = 0; i < column2Quantiles[column].size(); ++i) {
            auto q = column2Quantiles[column][i];
            const auto& borders = column2Borders[column][i];
            Cerr << "\t" << q << " : [" << borders.first << "," << borders.second << "]\n";
            column2Quantizers[column].emplace_back(borders.first, borders.second, outputMin, outputMax, q);
        }
        Cerr << "\n";
    }

    TVector<TString> quantileColumns;
    for (auto column : columns) {
        auto prefix = ToString(StringSplitter(column).Split('_').begin()->Token());
        quantileColumns.push_back(prefix + "_quantile");
    }

    if (queryTable == "") {
        CalcQuantizationMse(client, inputTable, outputTable, column2Quantizers, quantileColumns);
    } else {
        CalcQuantizationRankingMetricsWithQueries(client, inputTable, outputTable, queryTable, column2Quantizers,
            topSizes, contextWeight, replyColumns, threadCount, quantileColumns);
    }

    return 0;
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);
    TModChooser modChooser;
    modChooser.AddMode("apply", Apply, "Apply quantization");
    modChooser.AddMode("tune", TuneQuantiles, "Tune quantization quantiles");
    return modChooser.Run(argc, argv);
}
