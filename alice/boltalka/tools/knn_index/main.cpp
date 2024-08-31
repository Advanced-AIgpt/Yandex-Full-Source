#include <extsearch/images/quality/mr_kmeans/kmeans.h>
#include <extsearch/images/quality/mr_kmeans/vector.pb.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>
#include <library/cpp/knn_index/writer.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/operation.h>

#include <util/stream/file.h>
#include <util/string/split.h>
#include <util/generic/ymath.h>

using NKMeans::TFloatVector;

class TUniqueReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        Y_VERIFY(input->IsValid());
        output->AddRow(input->GetRow());
    }
};
REGISTER_REDUCER(TUniqueReducer);

int main_upload_table(int argc, const char** argv) {
    TString inputFilename;
    TString serverProxy;
    TString outputTable;

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption();
    opts
        .AddLongOption('p', "proxy")
        .Required()
        .StoreResult(&serverProxy)
        .Help("YT server.\n\n\n");
    opts
        .AddLongOption('i', "input")
        .Required()
        .StoreResult(&inputFilename);
    opts
        .AddLongOption('o', "output")
        .Required()
        .StoreResult(&outputTable);

    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);
    Y_UNUSED(parsedOpts);

    auto client = NYT::CreateClient(serverProxy);
    client->Create(outputTable, NYT::NT_TABLE, NYT::TCreateOptions().Recursive(true).IgnoreExisting(true));
    {
        auto writer = client->CreateTableWriter<NYT::TNode>(outputTable);

        TFileInput in(inputFilename);
        TString line;
        for (size_t id = 0; in.ReadLine(line); ++id) {
            TFloatVector vector;
            for (auto split : StringSplitter(line).SplitBySet(" \t").SkipEmpty()) {
                float value = FromString<float>(split.Token());
                vector.AddVector(value);
            }
            writer->AddRow(NYT::TNode()
                ("id", id)
                ("vector", vector.SerializeAsString())
            );
            size_t done = id + 1;
            if (done % 10000 == 0) {
                Cerr << "Processed " << done << " lines\r";
            }
        }
        Cerr << Endl;
        writer->Finish();
    }
    client->Sort(NYT::TSortOperationSpec().AddInput(outputTable).Output(outputTable).SortBy({ "vector", "id" }));
    client->Reduce(
        NYT::TReduceOperationSpec()
            .AddInput<NYT::TNode>(outputTable)
            .AddOutput<NYT::TNode>(outputTable)
            .ReduceBy({ "vector" }),
        new TUniqueReducer);
    client->Sort(NYT::TSortOperationSpec().AddInput(outputTable).Output(outputTable).SortBy({ "id" }));

    return 0;
}

int main_download_table(int argc, const char** argv) {
    TString inputTable;
    TString serverProxy;
    TString outputFilename;
    TString idsFilename;

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption();
    opts
        .AddLongOption('p', "proxy")
        .StoreResult(&serverProxy)
        .Help("YT server.\n\n\n");
    opts
        .AddLongOption('i', "input")
        .StoreResult(&inputTable)
        .Help("Input table\n\n\n");
    opts
        .AddLongOption('o', "output-filename")
        .StoreResult(&outputFilename)
        .Help("Output filename.\n\n\n");
    opts
        .AddLongOption('n', "ids-filename")
        .StoreResult(&idsFilename)
        .Help("If specified, save vector ids to this file.\n\n\n");

    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);
    Y_UNUSED(parsedOpts);

    auto client = NYT::CreateClient(serverProxy);
    auto reader = client->CreateTableReader<NYT::TNode>(inputTable);
    TFixedBufferFileOutput out(outputFilename);
    THolder<TFixedBufferFileOutput> idsOut;
    if (!idsFilename.empty()) {
        idsOut = MakeHolder<TFixedBufferFileOutput>(idsFilename);
    }
    for (; reader->IsValid(); reader->Next()) {
        TFloatVector vector;
        Y_PROTOBUF_SUPPRESS_NODISCARD vector.ParseFromString(reader->GetRow()["vector"].AsString());
        out.Write(vector.GetVector().data(), vector.GetVector().size() * sizeof(float));
        if (idsOut) {
            ui32 id = reader->GetRow()["id"].AsUint64();
            idsOut->Write(&id, sizeof(id));
        }
    }

    return 0;
}

int main_kmeans_clustering(int argc, const char** argv) {
    TString serverProxy;
    TString inputTable;
    TString outputTable;

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption();
    opts
        .AddLongOption('p', "proxy")
        .Required()
        .StoreResult(&serverProxy)
        .Help("YT server.\n\n\n");
    opts
        .AddLongOption('i', "input")
        .Required()
        .StoreResult(&inputTable);
    opts
        .AddLongOption('o', "output")
        .Required()
        .StoreResult(&outputTable);
    opts
        .AddCharOption('k')
        .DefaultValue("10000");
    opts
        .AddCharOption('n')
        .DefaultValue("30");

    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);
    Y_UNUSED(parsedOpts);

    auto client = NYT::CreateClient(serverProxy);
    NKMeans::KMeansClustering(client, inputTable, outputTable, parsedOpts.Get<size_t>('k'), parsedOpts.Get<size_t>('n'));

    return 0;
}

int main_kmeans_assignment(int argc, const char** argv) {
    TString serverProxy;
    TString inputTable;
    TString clusterTable;
    TString outputTable;

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption();
    opts
        .AddLongOption('p', "proxy")
        .Required()
        .StoreResult(&serverProxy)
        .Help("YT server.\n\n\n");
    opts
        .AddLongOption('i', "input")
        .Required()
        .StoreResult(&inputTable);
    opts
        .AddLongOption('c', "cluster")
        .Required()
        .StoreResult(&clusterTable);
    opts
        .AddLongOption('o', "output")
        .Required()
        .StoreResult(&outputTable);

    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);
    Y_UNUSED(parsedOpts);

    auto client = NYT::CreateClient(serverProxy);
    NKMeans::KMeansAssignment(client, inputTable, clusterTable, outputTable);

    return 0;
}

int main_build_index(int argc, const char** argv)
{
    TString serverProxy;
    TString inputTable;
    TString clusterTable;
    TString outputFilename;

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption();
    opts
        .AddLongOption('p', "proxy")
        .Required()
        .StoreResult(&serverProxy)
        .Help("YT server.\n\n\n");
    opts
        .AddLongOption('i', "input")
        .Required()
        .StoreResult(&inputTable);
    opts
        .AddLongOption('c', "cluster")
        .Required()
        .StoreResult(&clusterTable);
    opts
        .AddLongOption('o', "output")
        .Required()
        .StoreResult(&outputFilename);

    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);
    Y_UNUSED(parsedOpts);

    auto client = NYT::CreateClient(serverProxy);
    size_t dimension = 0;
    TVector<TVector<float>> clusters;
    {
        auto reader = client->CreateTableReader<NYT::TNode>(clusterTable);
        for (; reader->IsValid(); reader->Next()) {
            TFloatVector v;
            Y_PROTOBUF_SUPPRESS_NODISCARD v.ParseFromString(reader->GetRow()["vector"].AsString());
            ui32 clusterId = reader->GetRow()["cluster_id"].AsUint64();
            if (clusters.size() < clusterId + 1) {
                clusters.resize(clusterId + 1);
            }
            dimension = Max(dimension, v.VectorSize());
            clusters[clusterId].assign(v.GetVector().begin(), v.GetVector().end());
        }
    }
    {
        NKNNIndex::TWriter<float> writer(dimension, outputFilename);
        auto reader = client->CreateTableReader<NYT::TNode>(inputTable);
        TMaybe<ui32> lastClusterId;

        for (; reader->IsValid(); reader->Next()) {
            ui32 clusterId = reader->GetRow()["cluster_id"].AsUint64();
            if (lastClusterId && *lastClusterId != clusterId) {
                writer.WriteCluster(clusters[*lastClusterId].data());
            }
            lastClusterId = clusterId;
            ui32 id = reader->GetRow()["id"].AsUint64();
            TFloatVector v;
            Y_PROTOBUF_SUPPRESS_NODISCARD v.ParseFromString(reader->GetRow()["vector"].AsString());
            writer.WriteItem(id, 0, v.GetVector().data());
        }
        if (lastClusterId) {
            writer.WriteCluster(clusters[*lastClusterId].data());
        }
        writer.Finish();
    }
    return 0;
}

int main(int argc, const char** argv)
{
    NYT::Initialize(argc, argv);

    TModChooser modChooser;

    modChooser.AddMode(
        "download-table",
        main_download_table,
        "-- download table");

    modChooser.AddMode(
        "upload-table",
        main_upload_table,
        "-- upload table");

    modChooser.AddMode(
        "kmeans-clustering",
        main_kmeans_clustering,
        "-- kmeans clustering");

    modChooser.AddMode(
        "kmeans-assignment",
        main_kmeans_assignment,
        "-- kmeans assignment");

    modChooser.AddMode(
        "build-index",
        main_build_index,
        "-- build index");

    return modChooser.Run(argc, argv);
}
