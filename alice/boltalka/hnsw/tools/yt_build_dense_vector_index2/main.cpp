#include "distance.h"
#include "vector_component_type.h"

#include <alice/boltalka/hnsw/yt_index_builder2/dense_vector_index_builder.h>
#include <alice/boltalka/hnsw/yt_index_builder2/dense_vector_yt_jobs.h>
#include <alice/boltalka/hnsw/yt_index_builder2/index_writer.h>

#include <mapreduce/yt/client/init.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>

struct TOptions {
    NYtHnsw::THnswYtBuildOptions BuildOpts;
    TString ServerProxy;
    TString VectorTable;
    size_t Dimension;
    EVectorComponentType VectorComponentType = EVectorComponentType::Unknown;
    EDistance Distance = EDistance::Unknown;
    TString OutputTable;
    TString DocIdMappingTable;

    TOptions(int argc, const char** argv) {
        NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
        opts
            .AddHelpOption();
        opts
            .AddLongOption('p', "proxy")
            .RequiredArgument("STRING")
            .StoreResult(&ServerProxy)
            .Required()
            .Help("YT Server.");
        opts
            .AddLongOption('v', "vectors")
            .RequiredArgument("TABLE")
            .StoreResult(&VectorTable)
            .Required();
        opts
            .AddLongOption('t', "type")
            .RequiredArgument("STRING")
            .StoreResult(&VectorComponentType)
            .Required()
            .Help("One of { i8, i32, float }. Type of vectors' components.");
        opts
            .AddLongOption('d', "dim")
            .RequiredArgument("INT")
            .StoreResult(&Dimension)
            .Required();
        opts
            .AddLongOption('D', "distance")
            .RequiredArgument("STRING")
            .StoreResult(&Distance)
            .Required()
            .Help("One of { l1_distance, l2_sqr_distance, dot_product }.");
        opts
            .AddLongOption('o', "output")
            .RequiredArgument("TABLE")
            .StoreResult(&OutputTable)
            .Required();
        opts
            .AddLongOption('g', "docid-mapping")
            .RequiredArgument("TABLE")
            .StoreResult(&DocIdMappingTable)
            .Optional();
        opts
            .AddLongOption('T', "num-threads")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.NumThreads)
            .DefaultValue("8");
        opts
            .AddLongOption('m', "max-neighbors")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.MaxNeighbors)
            .DefaultValue("32");
        opts
            .AddLongOption('b', "batch-size")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.BatchSize)
            .DefaultValue("1000")
            .Help("Batch size. Affects performance.");
        opts
            .AddLongOption('u', "upper-level-batch-size")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.UpperLevelBatchSize)
            .DefaultValue("1000")
            .Help("Batch size for building upper levels. Affects accuracy.");
        opts
            .AddLongOption('s', "search-neighborhood-size")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.SearchNeighborhoodSize)
            .DefaultValue("400")
            .Help("Search neighborhood size for ANN-search");
        opts
            .AddLongOption('e', "num-exact-candidates")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.NumExactCandidates)
            .DefaultValue("100")
            .Help("Number of nearest vectors in batch.");
        opts
            .AddLongOption('l', "level-size-decay")
            .RequiredArgument("INT")
            .DefaultValue("max-neighbors / 2")
            .Help("Base of exponent for decaying level sizes.");
        opts
            .AddLongOption('M', "additional-memory-limit")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.AdditionalMemoryLimit)
            .DefaultValue("536870912")
            .Help("Additional memory limit for YT Jobs.");
        opts
            .AddLongOption('L', "max-items-for-local-build")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.MaxItemsForLocalBuild)
            .DefaultValue("200000")
            .Help("Upper (smaller) levels not exceeding max-items-for-local-build will be built in a single reduce job."
                  " Lower (bigger) levels will be split into ceil(level-size / max-items-for-local-build) partitions."
                  " Set this value so that building HNSW index of this size would take no more than tens of minutes.");
        opts
            .AddLongOption('c', "num-nearest-candidates")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.NumNearestCandidates)
            .DefaultValue("300")
            .Help("Number of nearest items to save from searching in each partition."
                  " Should be larger than max-neighbors because this candidates will be trimmed to max-neighbors."
                  " Larger values result in more accurate index.");
        opts
            .AddLongOption('j', "num-join-partitions-jobs")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.NumJoinPartitionsJobs)
            .DefaultValue("2000")
            .Help("Controls parallelization of searching for nearest items in partitions."
                  " Should be as big as you can afford.");
        opts
            .AddLongOption('r', "num-trim-neighbors-jobs")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.NumTrimNeighborsJobs)
            .DefaultValue("10")
            .Help("Controls parallelization of the last step - trimming neighbors");
        opts
            .AddLongOption("cpu-limit")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.CpuLimit)
            .DefaultValue("8")
            .Help("Might be used to lower guaranteed cpu requirements.");
        opts
            .AddLongOption("verbose")
            .NoArgument()
            .SetFlag(&BuildOpts.Verbose);

        opts.SetFreeArgsNum(0);
        opts.AddHelpOption('h');

        NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);

        if (!TryFromString<size_t>(parsedOpts.Get("level-size-decay"), BuildOpts.LevelSizeDecay)) {
            BuildOpts.LevelSizeDecay = NHnsw::THnswBuildOptions::AutoSelect;
        }
    }
};

template<class T, template<typename> class TDistance>
void BuildIndex(const TOptions& opts) {
    NYT::IClientPtr client = NYT::CreateClient(opts.ServerProxy);
    const TMaybe<TString> docIdMappingTable = opts.DocIdMappingTable.empty() ? Nothing() : TMaybe<TString>(opts.DocIdMappingTable);
    NYtHnsw::BuildDenseVectorIndex<T, TDistance<T>>(opts.BuildOpts, opts.Dimension, client, opts.VectorTable, opts.OutputTable, docIdMappingTable);
}

template<class T>
void DispatchDistance(const TOptions& opts) {
    switch (opts.Distance) {
        case EDistance::L1Distance: {
            BuildIndex<T, NHnsw::TL1Distance>(opts);
            break;
        }
        case EDistance::L2SqrDistance: {
            BuildIndex<T, NHnsw::TL2SqrDistance>(opts);
            break;
        }
        case EDistance::DotProduct: {
            BuildIndex<T, NHnsw::TDotProduct>(opts);
            break;
        }
        default: {
            Y_VERIFY(false, "Unknown distance!");
        }
    }
}

void DispatchVectorComponentType(const TOptions& opts) {
    switch (opts.VectorComponentType) {
        case EVectorComponentType::I8: {
            DispatchDistance<i8>(opts);
            break;
        }
        case EVectorComponentType::I32: {
            DispatchDistance<i32>(opts);
            break;
        }
        case EVectorComponentType::Float: {
            DispatchDistance<float>(opts);
            break;
        }
        default: {
            Y_VERIFY(false, "Unknown vector component type!");
        }
    }
}

void DistpatchAndBuildIndex(const TOptions& opts) {
    DispatchVectorComponentType(opts);
}

int main_build_index(int argc, const char** argv) {
    TOptions opts(argc, argv);

    DistpatchAndBuildIndex(opts);
    return 0;
}

REGISTER_COMMON_HNSW_JOBS();

int main_download_shard(int argc, const char** argv) {
    TString indexDataTable;
    TString outputFilename;
    TString serverProxy;
    size_t shardId;

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption();
    opts
        .AddLongOption('p', "proxy")
        .RequiredArgument("STRING")
        .Required()
        .StoreResult(&serverProxy)
        .Help("YT server.");
    opts
        .AddLongOption('i', "index-data")
        .RequiredArgument("TABLE")
        .Required()
        .StoreResult(&indexDataTable)
        .Help("Table with index data (output table of build mode)");
    opts
        .AddLongOption('o', "output")
        .RequiredArgument("FILE")
        .Required()
        .StoreResult(&outputFilename);
    opts
        .AddLongOption('s', "shard-id")
        .RequiredArgument("INT")
        .Required()
        .StoreResult(&shardId);

    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);

    auto client = NYT::CreateClient(serverProxy);
    NYtHnsw::WriteIndex(client, indexDataTable, shardId, outputFilename);
    return 0;
}

int main_download_all_shards(int argc, const char** argv) {
    TString indexDataTable;
    TString outputFilename;
    TString offsetsFilename;
    TString serverProxy;

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption();
    opts
        .AddLongOption('p', "proxy")
        .RequiredArgument("STRING")
        .Required()
        .StoreResult(&serverProxy)
        .Help("YT server.");
    opts
        .AddLongOption('i', "index-data")
        .RequiredArgument("TABLE")
        .Required()
        .StoreResult(&indexDataTable)
        .Help("Table with index data (output table of build mode)");
    opts
        .AddLongOption('o', "output")
        .RequiredArgument("FILE")
        .Required()
        .StoreResult(&outputFilename);
    opts
        .AddLongOption('f', "offsets-file")
        .RequiredArgument("FILE")
        .Required()
        .StoreResult(&offsetsFilename);

    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);

    auto client = NYT::CreateClient(serverProxy);

    TVector<ui64> offsets;
    NYtHnsw::WriteAllIndexes(client, indexDataTable, outputFilename, offsetsFilename);
    return 0;
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    TModChooser modChooser;

    modChooser.AddMode(
        "build",
        main_build_index,
        "-- build index");

    modChooser.AddMode(
        "download-shard",
        main_download_shard,
        "-- download shard");

    modChooser.AddMode(
        "download-all-shards",
        main_download_all_shards,
        "-- download-all-shards"
    );

    return modChooser.Run(argc, argv);
}

