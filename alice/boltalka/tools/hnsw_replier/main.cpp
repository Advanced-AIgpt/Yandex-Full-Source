#include <alice/boltalka/extsearch/base/search/knn_index.h>

#include <library/cpp/getopt/last_getopt.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/operation.h>

#include <util/folder/path.h>

class TMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TMapper() = default;
    TMapper(const TString& hnswPathPrefix,
            const TString& queryEmbeddingColumn,
            size_t embeddingDim,
            float contextWeight,
            size_t topSize,
            size_t knnSearchNeighborhoodSize,
            size_t knnDistanceCalcLimit)
        : HnswPathPrefix(hnswPathPrefix)
        , QueryEmbeddingColumn(queryEmbeddingColumn)
        , EmbeddingDim(embeddingDim)
        , ContextWeight(contextWeight)
        , TopSize(topSize)
        , KnnSearchNeighborhoodSize(knnSearchNeighborhoodSize)
        , KnnDistanceCalcLimit(knnDistanceCalcLimit)
    {
    }

    void Start(NYT::TTableWriter<NYT::TNode>*) override {
        KnnIndexPtr = new NNlg::TKnnIndex(HnswPathPrefix, 2 * EmbeddingDim, EMemoryMode::Locked);
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        Y_VERIFY(input->GetRow()[QueryEmbeddingColumn].AsString().length() / sizeof(float) == EmbeddingDim);
        for (; input->IsValid(); input->Next()) {
            auto row = input->GetRow();
            auto queryEmbedding = GetQueryEmbedding(row);
            auto results = KnnIndexPtr->FindNearestVectors(queryEmbedding.data(),
                                                           TopSize,
                                                           KnnSearchNeighborhoodSize,
                                                           KnnDistanceCalcLimit);
            row["shard_id"] = 0;
            for (const auto& result : results) {
                row["doc_id"] = result.DocId;
                row["inv_score"] = -result.Score;
                output->AddRow(row);
            }
        }
    }

    TVector<float> GetQueryEmbedding(const NYT::TNode& row) {
        TVector<float> queryEmbedding;
        const auto* contextEmbeddingPtr = reinterpret_cast<const float*>(row[QueryEmbeddingColumn].AsString().data());
        queryEmbedding.insert(queryEmbedding.end(), contextEmbeddingPtr, contextEmbeddingPtr + EmbeddingDim);
        queryEmbedding.insert(queryEmbedding.end(), contextEmbeddingPtr, contextEmbeddingPtr + EmbeddingDim);
        for (size_t i = 0; i < EmbeddingDim; ++i) {
            queryEmbedding[i] *= ContextWeight;
        }
        for (size_t i = EmbeddingDim; i < 2 * EmbeddingDim; ++i) {
            queryEmbedding[i] *= (1. - ContextWeight);
        }
        return queryEmbedding;
    }
    Y_SAVELOAD_JOB(HnswPathPrefix, QueryEmbeddingColumn, EmbeddingDim, ContextWeight, TopSize, KnnSearchNeighborhoodSize, KnnDistanceCalcLimit);

private:
    TString HnswPathPrefix;
    TString QueryEmbeddingColumn;
    size_t EmbeddingDim;
    float ContextWeight;
    size_t TopSize;
    size_t KnnSearchNeighborhoodSize;
    size_t KnnDistanceCalcLimit;
    NNlg::TKnnIndexPtr KnnIndexPtr;
};
REGISTER_MAPPER(TMapper);

int main_find_tops(int argc, const char** argv) {
    TString serverProxy;
    TString inputTable;
    TString outputTable;
    TString hnswIndex;
    TString hnswVectors;
    TString hnswIds;
    size_t embeddingDim;
    float contextWeight;
    size_t topSize;
    size_t knnSearchNeighborhoodSize;
    size_t knnDistanceCalcLimit;
    TString embeddingColumn;

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
        .AddLongOption("hnsw-index")
        .RequiredArgument("FILE")
        .StoreResult(&hnswIndex);
    opts
        .AddLongOption("hnsw-vectors")
        .RequiredArgument("FILE")
        .StoreResult(&hnswVectors);
    opts
        .AddLongOption("hnsw-ids")
        .RequiredArgument("FILE")
        .StoreResult(&hnswIds);
    opts
        .AddLongOption("dim")
        .RequiredArgument("INT")
        .StoreResult(&embeddingDim);
    opts
        .AddLongOption("context-weight")
        .RequiredArgument("FLOAT")
        .DefaultValue(0.5)
        .StoreResult(&contextWeight);
    opts
        .AddLongOption("top-size")
        .RequiredArgument("INT")
        .Required()
        .StoreResult(&topSize);
    opts
        .AddLongOption("search-neighborhood-size")
        .RequiredArgument("INT")
        .DefaultValue(500)
        .StoreResult(&knnSearchNeighborhoodSize);
    opts
        .AddLongOption("distance-calc-limit")
        .RequiredArgument("INT")
        .DefaultValue(8500)
        .StoreResult(&knnDistanceCalcLimit);
    opts
        .AddLongOption("embedding-column")
        .DefaultValue("context_embedding")
        .StoreResult(&embeddingColumn);
    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');
    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);

    auto client = NYT::CreateClient(serverProxy);
    client->Create(outputTable, NYT::NT_TABLE, NYT::TCreateOptions().Recursive(true).IgnoreExisting(true));

    const ui64 memoryLimit = (1ULL << 27) + GetFileLength(hnswIndex) + GetFileLength(hnswVectors) + GetFileLength(hnswIds);
    const TString hnswPathPrefix = "hnsw";
    auto userJobSpec = NYT::TUserJobSpec()
        .AddLocalFile(hnswIndex, NYT::TAddLocalFileOptions().PathInJob(hnswPathPrefix + ".index"))
        .AddLocalFile(hnswVectors, NYT::TAddLocalFileOptions().PathInJob(hnswPathPrefix + ".vec"))
        .AddLocalFile(hnswIds, NYT::TAddLocalFileOptions().PathInJob(hnswPathPrefix + ".ids"))
        .MemoryLimit(memoryLimit);

    auto mapSpec = NYT::TMapOperationSpec()
        .AddInput<NYT::TNode>(inputTable)
        .MapperSpec(userJobSpec);

    if (client->Get(inputTable + "/@sorted").AsBool()) {
        auto nodes = client->Get(inputTable + "/@sorted_by").AsList();
        TVector<TString> sortedBy;
        for (const auto& node : nodes) {
            sortedBy.push_back(node.AsString());
        }
        sortedBy.push_back("inv_score");
        mapSpec.AddOutput<NYT::TNode>(NYT::TRichYPath(outputTable).SortedBy(sortedBy)).Ordered(true);
    } else {
        mapSpec.AddOutput<NYT::TNode>(outputTable);
    }

    client->Map(
        mapSpec,
        new TMapper(hnswPathPrefix, embeddingColumn, embeddingDim, contextWeight, topSize, knnSearchNeighborhoodSize, knnDistanceCalcLimit)
    );
    return 0;
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);
    return main_find_tops(argc, argv);
}
