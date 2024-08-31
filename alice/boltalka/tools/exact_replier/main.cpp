#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/dot_product/dot_product.h>
#include <library/cpp/threading/local_executor/local_executor.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/operation.h>

#include <util/generic/queue.h>
#include <util/stream/str.h>
#include <util/stream/file.h>
#include <util/string/join.h>
#include <util/ysaveload.h>

namespace {

static TString ParseContext(const NYT::TNode& row, const TString& eol = "\n") {
    TVector<TString> context;
    for (size_t i = 0; ; ++i) {
        TString key = "context_" + ToString(i);
        if (!row.HasKey(key)) {
            break;
        }
        context.push_back(row[key].AsString());
    }
    std::reverse(context.begin(), context.end());
    return JoinSeq(eol, context);
}

static TVector<float> ParseEmbedding(const NYT::TNode& row, const TString& columnName) {
    const auto& embeddingStr = row[columnName].AsString();
    return {reinterpret_cast<const float*>(embeddingStr.data()), reinterpret_cast<const float*>(embeddingStr.data() + embeddingStr.size())};
}

static TString Escape(const TString& s) {
    TString res;
    for (char c : s) {
        switch (c) {
            case '\r': {
                res += '\\';
                res += 'r';
                break;
            }
            case '\n': {
                res += '\\';
                res += 'n';
                break;
            }
            default: {
                res += c;
            }
        }
    }
    return res;
}

}

struct TContext {
    ui64 Id;
    TString Context;
    TVector<TVector<float> > Embeddings;

    Y_SAVELOAD_DEFINE(Id, Context, Embeddings);
};

struct TReply {
    TString Context;
    TString Reply;
    TString RewrittenReply;
    float Score;
    ui64 ShardId;
    ui64 DocId;

    Y_SAVELOAD_DEFINE(Context, Reply, RewrittenReply, Score, ShardId, DocId);
};

struct TReplyBetter {
    bool operator()(const TReply& a, const TReply& b) const {
        return a.Score > b.Score;
    }
};


class TReplyMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TReplyMapper() = default;

    TReplyMapper(const TVector<TContext>& contexts, float contextWeight, size_t topSize, size_t threadCount, TVector<TString> factorWeights)
        : Contexts(contexts)
        , ContextWeight(contextWeight)
        , TopSize(topSize)
        , ThreadCount(threadCount)
        , FactorWeights(factorWeights)
    {
    }

    void Start(NYT::TTableWriter<NYT::TNode>*) override {
        NPar::LocalExecutor().RunAdditionalThreads(ThreadCount - 1);
        TopReplies.clear();
        TopReplies.resize(Contexts.size());
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();
            const TReply reply = {
                ParseContext(row, "\t"),
                row["reply"].AsString(),
                row.HasKey("rewritten_reply") ? row["rewritten_reply"].AsString() : row["reply"].AsString(),
                0.f,
                row["shard_id"].AsUint64(),
                row["doc_id"].AsUint64()
            };
            auto task = [&](int id) {
                const size_t dimension = Contexts[id].Embeddings[0].size();
                float score = 0;
                for (size_t factorIdx = 0; factorIdx < FactorWeights.size(); ++factorIdx) {
                    TString prefix = "";
                    if (factorIdx > 0) {
                        prefix = "factor_dssm_" + ToString(factorIdx - 1) + ":";
                    }
                    const float* contextEmbedding = reinterpret_cast<const float*>(row[prefix + "context_embedding"].AsString().data());
                    const float* replyEmbedding = reinterpret_cast<const float*>(row[prefix + "reply_embedding"].AsString().data());
                    float delta = DotProduct(Contexts[id].Embeddings[factorIdx].data(), contextEmbedding, dimension) * ContextWeight
                        + DotProduct(Contexts[id].Embeddings[factorIdx].data(), replyEmbedding, dimension) * (1.0 - ContextWeight);
                    score += delta * FromString<float>(FactorWeights[factorIdx]);
                }
                if (TopReplies[id].size() < TopSize || TopReplies[id].top().Score < score) {
                    auto copyReply = reply;
                    copyReply.Score = score;
                    TopReplies[id].push(copyReply);
                    if (TopReplies[id].size() > TopSize) {
                        TopReplies[id].pop();
                    }
                }
            };
            NPar::LocalExecutor().ExecRange(task, 0, Contexts.size(), NPar::TLocalExecutor::WAIT_COMPLETE);
        }
        for (size_t i = 0; i < Contexts.size(); ++i) {
            TVector<TReply> replies;
            for (; !TopReplies[i].empty(); TopReplies[i].pop()) {
                replies.push_back(TopReplies[i].top());
            }
            TString repliesStr;
            {
                TStringOutput stringOutput(repliesStr);
                ::Save(&stringOutput, replies);
            }
            output->AddRow(NYT::TNode()
                ("context_id", i)
                ("context", Contexts[i].Context)
                ("replies", repliesStr)
            );
        }
    }

    Y_SAVELOAD_JOB(Contexts, ContextWeight, TopSize, ThreadCount, FactorWeights);

private:
    TVector<TContext> Contexts;
    float ContextWeight;
    ui64 TopSize;
    ui64 ThreadCount;
    TVector<TString> FactorWeights;

    TVector<TPriorityQueue<TReply, TVector<TReply>, TReplyBetter>> TopReplies;
};
REGISTER_MAPPER(TReplyMapper);

class TReplyReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TReplyReducer() = default;

    TReplyReducer(size_t topSize)
        : TopSize(topSize)
    {
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        const ui64 contextId = input->GetRow()["context_id"].AsUint64();
        const TString context = input->GetRow()["context"].AsString();

        TPriorityQueue<TReply, TVector<TReply>, TReplyBetter> topReplies;
        for (; input->IsValid(); input->Next()) {
            TVector<TReply> replies;
            {
                TStringInput stringInput(input->GetRow()["replies"].AsString());
                ::Load(&stringInput, replies);
            }
            for (const auto& reply : replies) {
                if (topReplies.size() < TopSize || topReplies.top().Score < reply.Score) {
                    topReplies.push(reply);
                    if (topReplies.size() > TopSize) {
                        topReplies.pop();
                    }
                }
            }
        }
        for (; !topReplies.empty(); topReplies.pop()) {
            const auto& reply = topReplies.top();
            output->AddRow(NYT::TNode()
                ("context_id", contextId)
                ("context", context)
                ("reply", reply.Reply)
                ("rewritten_reply", reply.RewrittenReply)
                ("reply_context", reply.Context)
                ("score", reply.Score)
                ("shard_id", reply.ShardId)
                ("doc_id", reply.DocId)
                ("inv_score", -reply.Score)
            );
        }
    }

    Y_SAVELOAD_JOB(TopSize);

private:
    ui64 TopSize;
};
REGISTER_REDUCER(TReplyReducer);

TVector<TContext> ReadContexts(NYT::IClientPtr client, const TString& contextTable, size_t factorNum) {
    auto input = client->CreateTableReader<NYT::TNode>(contextTable);
    TVector<TContext> contexts;
    for (; input->IsValid(); input->Next()) {
        ui64 id = contexts.size();
        contexts.emplace_back();
        auto& context = contexts.back();
        context.Id = id;
        context.Context = ParseContext(input->GetRow(), "\t");
        context.Embeddings.push_back(ParseEmbedding(input->GetRow(), "context_embedding"));
        for (size_t factorIdx = 0; factorIdx + 1 < factorNum; ++factorIdx) {
            context.Embeddings.push_back(ParseEmbedding(input->GetRow(), "factor_dssm_" + ToString(factorIdx) + ":context_embedding"));
        }
    }
    return contexts;
}

void WriteTsvResult(NYT::IClientPtr client, const TString& resultTable, const TString& outputFilename) {
    auto input = client->CreateTableReader<NYT::TNode>(resultTable);
    TFixedBufferFileOutput output(outputFilename);
    for (; input->IsValid(); input->Next()) {
        const auto& row = input->GetRow();
        float score = row["score"].AsDouble();
        TString context = row["context"].AsString();
        TString reply = row["rewritten_reply"].AsString();
        output << score << '\t' << Escape(context) << '\t' << Escape(reply) << '\n';
    }
}

int main_replier(int argc, const char** argv) {
    TString serverProxy;
    TString contextTable;
    TString replyTable;
    TString outputTable;
    size_t numShards;
    float contextWeight;
    size_t threadCount;
    TString factorWeightsString;
    TVector<TString> factorWeights;
    size_t topSize;
    TString outputFilename;

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption();
    opts
        .AddLongOption('p', "proxy")
        .DefaultValue("hahn")
        .StoreResult(&serverProxy)
        .Help("YT server.\n\n\n");
    opts
        .AddLongOption('c', "contexts")
        .RequiredArgument("TABLE")
        .Required()
        .StoreResult(&contextTable);
    opts
        .AddLongOption('r', "replies")
        .RequiredArgument("TABLE")
        .Required()
        .StoreResult(&replyTable);
    opts
        .AddLongOption('o', "output-table")
        .RequiredArgument("TABLE")
        .Required()
        .StoreResult(&outputTable);
    opts
        .AddLongOption('O', "output")
        .RequiredArgument("FILE")
        .Required()
        .StoreResult(&outputFilename)
        .Help("Output file with TSV-result.");
    opts
        .AddLongOption('n', "num-shards")
        .RequiredArgument("INT")
        .Optional()
        .DefaultValue("1")
        .StoreResult(&numShards)
        .Help("Number of first shards to use.");
    opts
        .AddLongOption('w', "context-weight")
        .RequiredArgument("FLOAT")
        .Optional()
        .DefaultValue("0.5")
        .StoreResult(&contextWeight)
        .Help("Reply score is computed as: contextWeight * dot(query, context) + (1.0 - contextWeight) * dot(query, reply)");
    opts
        .AddLongOption('t', "top")
        .RequiredArgument("INT")
        .Optional()
        .DefaultValue("10")
        .StoreResult(&topSize);
    opts
        .AddLongOption('T', "thread-count")
        .RequiredArgument("INT")
        .Optional()
        .DefaultValue("8")
        .StoreResult(&threadCount);
    opts
        .AddLongOption('W', "factor-weights")
        .RequiredArgument("STRING")
        .Optional()
        .DefaultValue("1")
        .StoreResult(&factorWeightsString);

    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');
    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);

    Split(factorWeightsString, ",", factorWeights);
    auto client = NYT::CreateClient(serverProxy);
    auto contexts = ReadContexts(client, contextTable, factorWeights.size());

    client->Create(outputTable, NYT::NT_TABLE, NYT::TCreateOptions().Recursive(true).IgnoreExisting(true));

    const ui64 memoryLimit = (1ULL << 30);
    client->MapReduce(
        NYT::TMapReduceOperationSpec()
            .AddInput<NYT::TNode>(NYT::TRichYPath(replyTable)
                .AddRange(NYT::TReadRange()
                    .LowerLimit(NYT::TReadLimit().Key(0))
                    .UpperLimit(NYT::TReadLimit().Key(numShards))))
            .AddOutput<NYT::TNode>(outputTable)
            .MapperSpec(NYT::TUserJobSpec().MemoryLimit(memoryLimit))
            .ReduceBy({"context_id"}),
        new TReplyMapper(contexts, contextWeight, topSize, threadCount, factorWeights),
        new TReplyReducer(topSize),
        NYT::TOperationOptions().Spec(NYT::TNode()
            ("mapper", NYT::TNode()("cpu_limit", threadCount))));

    client->Sort(
        NYT::TSortOperationSpec()
            .AddInput(outputTable)
            .Output(outputTable)
            .SortBy({"context_id", "inv_score"}));

    WriteTsvResult(client, outputTable, outputFilename);

    return 0;
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    return main_replier(argc, argv);
}
