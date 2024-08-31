#include <library/cpp/dot_product/dot_product.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/operation.h>

#include <alice/boltalka/libs/nlgsearch_simple/nlgsearch_simple.h>

#include <util/string/join.h>
#include <util/system/shellcommand.h>

TString SafeAsString(const NYT::TNode& node) {
    return node.IsString() ? node.AsString() : "";
}

class TReplyMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TReplyMapper() = default;

    TReplyMapper(TString index, ui64 threadCount) : Index(index), ThreadCount(threadCount)
    {
    }

    void Start(NYT::TTableWriter<NYT::TNode>*) override {
        //NPar::LocalExecutor().RunAdditionalThreads(ThreadCount - 1);
        Y_VERIFY(TShellCommand("sky", {"get", "rbtorrent:751105d2e88132beb9d272950e781cc28b1444b8"}, TShellCommandOptions().SetInheritError(true)).Run().GetStatus() == TShellCommand::SHELL_FINISHED, "sky get failed");
        NNlg::TNlgSearchSimpleParams params;
        params.IndexDir = Index;
        params.KnnIndexNames = "base:sns1400:dcl35000,assessors:sns500:dcl8500";
        params.BaseDssmModelName = "insight_c3_rus_lister";
        params.BaseKnnIndexName = "base";
        params.MaxResults = 50;
        params.NumThreads = ThreadCount;
        SearcherPtr = new NNlg::TNlgSearchSimple(params);
    }

    static TString EmbeddingToString(const float* embedding, size_t dimension) {
        return {reinterpret_cast<const char*>(embedding), dimension * sizeof(float)};
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        for (; input->IsValid(); input->Next()) {
            const size_t dim = 300;
            const auto& row = input->GetRow();
            const TVector<TString> context = {SafeAsString(row["context_2"]), SafeAsString(row["context_1"]), SafeAsString(row["context_0"])};
            const TString reply = SafeAsString(row["reply"]);
            TVector<const float*> replyEmbeddings;
            TVector<TString> replies;
            TVector<float> contextEmbedding;
            TVector<float> relevs;
            SearcherPtr->GetCandidates("insight_c3_rus_lister", context, &replyEmbeddings, &replies, &contextEmbedding, &relevs);
            TVector<float> currentReplyEmbedding = SearcherPtr->EmbedReply("insight_c3_rus_lister", reply);
            TVector<float> distances;
            distances.reserve(replyEmbeddings.size());
            for (const float* el : replyEmbeddings) {
                distances.push_back(DotProduct(currentReplyEmbedding.data(), el, dim));
            }
            size_t bestReplyIdx = MaxElement(distances.begin(), distances.end()) - distances.begin();
            TVector<float> candidateEmbeddings;
            replies.erase(replies.begin() + bestReplyIdx);
            candidateEmbeddings.reserve(replyEmbeddings.size() * dim);
            for (size_t i = 0; i < replyEmbeddings.size(); ++i) {
                if (i != bestReplyIdx) {
                    auto replyBegin = replyEmbeddings[i];
                    std::copy(replyBegin, replyBegin + dim, std::back_inserter(candidateEmbeddings));
                }
            }
            output->AddRow(NYT::TNode()
                ("context", JoinRange("\t", context.begin(), context.end()))
                ("reply", reply)
                ("context_embedding", EmbeddingToString(contextEmbedding.data(), dim))
                ("reply_embedding", EmbeddingToString(currentReplyEmbedding.data(), dim))
                ("candidate_embeddings", EmbeddingToString(candidateEmbeddings.data(), candidateEmbeddings.size()))
                ("candidates", JoinRange("\t", replies.begin(), replies.end()))
            );
        }
    }

    Y_SAVELOAD_JOB(Index, ThreadCount);

private:
    TString Index;
    ui64 ThreadCount;
    NNlg::TNlgSearchSimplePtr SearcherPtr;
};
REGISTER_MAPPER(TReplyMapper);



int main(int argc, const char* argv[])
{
    NYT::Initialize(argc, argv);
    auto client = NYT::CreateClient("hahn");
    const ui64 memoryLimit = (60ULL << 30);
    auto userJobSpec = NYT::TUserJobSpec()
        //.AddLocalFile("/mnt/storage/nzinov/rl/index", NYT::TAddLocalFileOptions().PathInJob("index"))
        .MemoryLimit(memoryLimit)
        .ExtraTmpfsSize(45ULL << 30);
    client->Map(
        NYT::TMapOperationSpec()
            .AddInput<NYT::TNode>(NYT::TRichYPath("//home/voice/krom/sessions_29092019/gc_c3_searchappprod"))
            .AddOutput<NYT::TNode>(NYT::TRichYPath("//home/voice/nzinov/policy_approximator/dataset"))
            .DataSizePerJob(10 * 1024 * 1024)
            .MaxFailedJobCount(200)
            .MapperSpec(userJobSpec),
        new TReplyMapper("index", 1),
        NYT::TOperationOptions().MountSandboxInTmpfs(true).Spec(NYT::TNode()("scheduling_tag_filter", "yp_lite | iss"))
    );
    return 0;
}
