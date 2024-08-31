#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/hnsw/index/dense_vector_index.h>
#include <library/cpp/hnsw/index_builder/dense_vector_index_builder.h>
#include <library/cpp/hnsw/index_builder/index_writer.h>


NHnsw::THnswBuildOptions SkipListBuildOptions() {
    NHnsw::THnswBuildOptions opts;
    opts.MaxNeighbors = 32;
    opts.BatchSize = 900;
    opts.UpperLevelBatchSize = 36000;
    opts.SearchNeighborhoodSize = 270;
    opts.NumExactCandidates = 90;
    opts.NumThreads = 45;
    opts.LevelSizeDecay = 2;
    return opts;
}


int main(int argc, char** argv) {
    auto indexOpts = SkipListBuildOptions();

    TString vecPath;
    TString outIndexPath;
    size_t embeddingSize;

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption();
    opts
        .AddLongOption("vec-path")
        .RequiredArgument("FILE")
        .Required()
        .StoreResult(&vecPath);
    opts
        .AddLongOption("out-index-path")
        .RequiredArgument("FILE")
        .Required()
        .StoreResult(&outIndexPath);
    opts
        .AddLongOption("embedding-size")
        .RequiredArgument("INT")
        .DefaultValue(256)
        .StoreResult(&embeddingSize);
    
    NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    Cout << "Vec path: " << vecPath << Endl;
    Cout << "Starting to build index..." << Endl;
    auto indexData = NHnsw::BuildDenseVectorIndex<float, NHnsw::TDotProduct<float>>(indexOpts, vecPath, embeddingSize);
    Cout << "Starting to save index..." << Endl;
    TFileOutput out(outIndexPath);
    NHnsw::WriteIndex(indexData, out);
    out.Flush();
    return 0;
}
