#include "distance.h"
#include "vector_component_type.h"

#include <library/cpp/hnsw/index/dense_vector_distance.h>
#include <alice/boltalka/hnsw/yt_index_builder/dense_vector_index_builder.h>
#include <alice/boltalka/hnsw/yt_index_builder/index_writer.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>

struct TOptions {
    NHnsw::THnswYtBuildOptions BuildOpts;
    TString ServerProxy;
    TString VectorTable;
    size_t Dimension;
    EVectorComponentType VectorComponentType = EVectorComponentType::Unknown;
    EDistance Distance = EDistance::Unknown;
    TString OutputTable;

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
        opts.
            AddLongOption('o', "output")
            .RequiredArgument("TABLE")
            .StoreResult(&OutputTable)
            .Required();
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
            .DefaultValue("250000")
            .Help("Batch size. Affects performance.");
        opts
            .AddLongOption('u', "upper-level-batch-size")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.UpperLevelBatchSize)
            .DefaultValue("500000")
            .Help("Batch size for building upper levels. Affects accuracy.");
        opts
            .AddLongOption('s', "search-neighborhood-size")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.SearchNeighborhoodSize)
            .DefaultValue("1000")
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
            .AddLongOption("ann-search-memory-limit")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.AnnSearchMemoryLimit)
            .DefaultValue("21474836480");
        opts
            .AddLongOption("ann-search-item-storage-memory-limit")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.AnnSearchItemStorageMemoryLimit)
            .DefaultValue("12884901888");
        opts
            .AddLongOption("ann-search-job-count")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.AnnSearchJobCount)
            .DefaultValue("100");
        opts
            .AddLongOption("exact-search-memory-limit")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.ExactSearchMemoryLimit)
            .DefaultValue("4294967296");
        opts
            .AddLongOption("exact-search-item-storage-memory-limit")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.ExactSearchItemStorageMemoryLimit)
            .DefaultValue("1073741824");
        opts
            .AddLongOption("exact-search-job-count")
            .RequiredArgument("INT")
            .StoreResult(&BuildOpts.ExactSearchJobCount)
            .DefaultValue("100");
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
    NHnsw::BuildDenseVectorIndex<T, TDistance<T>>(opts.BuildOpts, opts.Dimension, client, opts.VectorTable, opts.OutputTable);
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

#define REGISTER_JOBS(Type, TDistance) \
    using T##Type##TDistance##Traits = NHnsw::TDenseDistanceTraits<Type, NHnsw::TDistance<Type>>; \
    REGISTER_HNSW_YT_DENSE_VECTOR_BUILD_JOBS(T##Type##TDistance##Traits, NHnsw::TYtDenseVectorStorage<Type>);

#define REGISTER_JOBS_WITH_TYPE(Type) \
    REGISTER_JOBS(Type, TL1Distance); \
    REGISTER_JOBS(Type, TL2SqrDistance); \
    REGISTER_JOBS(Type, TDotProduct);

REGISTER_JOBS_WITH_TYPE(i8);
REGISTER_JOBS_WITH_TYPE(i32);
REGISTER_JOBS_WITH_TYPE(float);

int main_build_index(int argc, const char** argv) {
    TOptions opts(argc, argv);

    DistpatchAndBuildIndex(opts);
    return 0;
}

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
    NHnsw::WriteIndex(client, indexDataTable, shardId, outputFilename);
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

    return modChooser.Run(argc, argv);
}

