#include <alice/boltalka/libs/text_utils/context_transform.h>
#include <alice/boltalka/libs/dssm_model/dssm_model.h>
#include <alice/boltalka/extsearch/base/calc_factors/factor_calcer.h>
#include <alice/boltalka/extsearch/base/calc_factors/nlg_search_factor_calcer.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/langs/langs.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/operation.h>

#include <contrib/libs/intel/mkl/include/mkl.h>

#include <util/digest/city.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/system/fstat.h>
#include <util/generic/list.h>

namespace {

template<class T, class U = T>
static TVector<U> ParseByPrefix(const NYT::TNode& row, const TString& prefix) {
    TVector<U> result;
    for (size_t i = 0; ; ++i) {
        TString key = prefix + ToString(i);
        if (!row.HasKey(key)) {
            break;
        }
        result.push_back(row[key].ConvertTo<T>());
    }
    return result;
}

}

class TCalcFactorsReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TCalcFactorsReducer() = default;

    TCalcFactorsReducer(const TString& modelFilename,
                        const TVector<TString>& factorDssmFilenames,
                        size_t numBaseDssmIndexes,
                        const TString& rusListerMapFilename,
                        ELanguage lang)
        : NumBaseDssmIndexes(numBaseDssmIndexes)
        , RusListerMapFilename(TFsPath(rusListerMapFilename).Basename())
        , Lang(lang)
    {
        DssmModelNames.push_back(TFsPath(modelFilename).Basename());
        EmbeddingColumnPrefixes.push_back("");
        for (size_t i = 0; i < factorDssmFilenames.size(); ++i) {
            DssmModelNames.push_back(TFsPath(factorDssmFilenames[i]).Basename());
            EmbeddingColumnPrefixes.push_back("factor_dssm_" + ToString(i) + ":");
        }
        Y_ASSERT(NumBaseDssmIndexes <= DssmModelNames.size());
    }

    void Start(NYT::TTableWriter<NYT::TNode>*) override {
        mkl_set_num_threads(1);
        mkl_cbwr_set(MKL_CBWR_COMPATIBLE);

        for (const auto& modelName : DssmModelNames) {
            DssmModels.push_back(new NNlg::TDssmModel(modelName));
        }

        const auto& baseDssmModelNames = TVector<TString>(DssmModelNames.begin(), DssmModelNames.begin() + NumBaseDssmIndexes);
        const auto& factorDssmModelNames = TVector<TString>(DssmModelNames.begin() + NumBaseDssmIndexes, DssmModelNames.end());
        FactorCalcer = NNlg::CreateNlgSearchFactorCalcer(RusListerMapFilename, baseDssmModelNames, factorDssmModelNames);
        ReplyTransform = new NNlgTextUtils::TNlgSearchUtteranceTransform(Lang);
        ContextTransform = new NNlgTextUtils::TNlgSearchContextTransform(Lang);
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        TVector<TVector<TString>> contexts;
        TVector<TString> replies;
        NNlg::TFactorCalcerCtx ctx(contexts, replies);

        // as DssmFactorsCtx does not own data we need to store embeddings separately
        TVector<NYT::TNode> rows;
        TList<TVector<float>> dssmQueryEmbeddings;

        TVector<TVector<float>> staticFactors;

        size_t numStaticFactors = 0;
        for (; input->IsValid(); input->Next()) {
            rows.push_back(input->GetRow());
            const auto& row = rows.back();

            auto context = ContextTransform->Transform(ParseByPrefix<TString>(row, "context_"));
            auto reply = ReplyTransform->Transform(row["reply"].AsString());
            ctx.Contexts.push_back(context);
            ctx.Replies.push_back(reply);

            auto query = ContextTransform->Transform(ParseByPrefix<TString>(row, "query_"));
            if (ctx.QueryContext.empty()) {
                ctx.QueryContext = query;
                for (size_t i = 0; i < DssmModels.size(); ++i) {
                    dssmQueryEmbeddings.emplace_back();
                    AddQueryEmbeddings(DssmModels[i], DssmModelNames[i], query, &dssmQueryEmbeddings.back(), &ctx);
                }
            }
            Y_VERIFY(ctx.QueryContext == query);

            for (size_t i = 0; i < DssmModelNames.size(); ++i) {
                AddContextAndReplyEmbeddings(row, DssmModelNames[i], EmbeddingColumnPrefixes[i], &ctx);
            }

            staticFactors.push_back(ParseByPrefix<double, float>(row, "static_factor_"));
            Y_VERIFY(numStaticFactors == 0 || staticFactors.back().size() == numStaticFactors);
            ctx.StaticFactors.push_back(staticFactors.back().data());
            numStaticFactors = staticFactors.back().size();

            size_t dssmId = row["dssm_id"].AsInt64();
            Y_VERIFY(dssmId < DssmModelNames.size());
            ctx.DssmIndexNames.push_back(DssmModelNames[dssmId]);
            ctx.KnnIndexNames.push_back(row["index_name"].AsString());
        }
        ctx.NumStaticFactors = numStaticFactors;
        auto factors = staticFactors;
        FactorCalcer->CalcFactors(&ctx, &factors);
        for (size_t i = 0; i < rows.size(); ++i) {
            auto row = rows[i];
            for (size_t j = 0; j < factors[i].size(); ++j) {
                row["factor_" + ToString(j)] = factors[i][j];
            }
            output->AddRow(row);
        }
    }
    Y_SAVELOAD_JOB(DssmModelNames, EmbeddingColumnPrefixes, NumBaseDssmIndexes, RusListerMapFilename, Lang);

private:
    static void AddQueryEmbeddings(const NNlg::TDssmModelPtr& dssmModel,
                                   const TString& dssmModelName,
                                   const TVector<TString>& query,
                                   TVector<float>* queryEmbedding,
                                   NNlg::TFactorCalcerCtx* ctx) {
        auto& dssmCtx = ctx->DssmFactorCtxs[dssmModelName];
        *queryEmbedding = dssmModel->Fprop(query, "", { "query_embedding" })[0];
        dssmCtx.Dimension = queryEmbedding->size();
        dssmCtx.QueryEmbedding = queryEmbedding->data();
    }

    static void AddContextAndReplyEmbeddings(const NYT::TNode& row,
                                             const TString& dssmModelName,
                                             const TString& columnPrefix,
                                             NNlg::TFactorCalcerCtx* ctx) {
        auto& dssmCtx = ctx->DssmFactorCtxs[dssmModelName];
        dssmCtx.ContextEmbeddings.push_back(ParseEmbedding(row, columnPrefix + "context_embedding"));
        dssmCtx.ReplyEmbeddings.push_back(ParseEmbedding(row, columnPrefix + "reply_embedding"));
    }

    static const float* ParseEmbedding(const NYT::TNode& row, const TString& name) {
        return reinterpret_cast<const float*>(row[name].AsString().data());
    }

private:
    TVector<TString> DssmModelNames;
    TVector<TString> EmbeddingColumnPrefixes;
    size_t NumBaseDssmIndexes;
    TString RusListerMapFilename;
    TVector<NNlg::TDssmModelPtr> DssmModels;
    ELanguage Lang;
    NNlg::TFactorCalcerPtr FactorCalcer;
    NNlgTextUtils::IUtteranceTransformPtr ReplyTransform;
    NNlgTextUtils::IContextTransformPtr ContextTransform;
};
REGISTER_REDUCER(TCalcFactorsReducer);

int main_calc_factors(int argc, const char** argv) {
    TString serverProxy;
    TString inputTable;
    TString outputTable;
    TString modelFilename;
    TString rusListerMapFilename;
    ELanguage lang;
    ui32 jobCount;
    size_t numBaseDssmIndexes;

    ui64 memoryLimit = (2ULL << 31);
    ui64 maxModelSize = 0;
    TVector<TString> factorDssmFilenames;
    auto reducerSpec = NYT::TUserJobSpec();

    auto addModelHandler = [&](const TString& filename) {
        reducerSpec.AddLocalFile(filename);
        ui64 modelSize = GetFileLength(filename);
        maxModelSize = Max(maxModelSize, modelSize);
        memoryLimit += modelSize;
    };

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption();
    opts
        .AddLongOption('p', "proxy")
        .RequiredArgument("STRING")
        .DefaultValue("hahn")
        .StoreResult(&serverProxy)
        .Help("YT server.");
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
        .AddLongOption('l', "lang")
        .RequiredArgument("LANGUAGE")
        .DefaultValue("ru")
        .Handler1T<TString>([&](const TString& language) {
            lang = LanguageByNameOrDie(language);
        })
        .Help("Language");
    opts
        .AddLongOption('m', "model")
        .RequiredArgument("FILE")
        .Required()
        .Handler1T<TString>([&](const TString& filename) {
            modelFilename = filename;
            addModelHandler(filename);
        })
        .Help("Dssm model file.");
    opts
        .AddLongOption('r', "rus-lister-map")
        .RequiredArgument("FILE")
        .Required()
        .StoreResult(&rusListerMapFilename)
        .Help("File with rus lister pos-sex info.");
    opts
        .AddLongOption('j', "job-count")
        .RequiredArgument("INT")
        .DefaultValue(200)
        .StoreResult(&jobCount);
    opts.
        AddLongOption('f', "factor-dssm")
        .RequiredArgument("FILE")
        .Handler1T<TString>([&](const TString& filename) {
            factorDssmFilenames.push_back(filename);
            addModelHandler(filename);
        })
        .Help("Factor dssm model file. You can add several models by repeating this option.");
    opts.
        AddLongOption("num-base-dssm-indexes")
        .RequiredArgument("INT")
        .DefaultValue(1)
        .StoreResult(&numBaseDssmIndexes)
        .Help("Model and first (num-base-dssm-indexes - 1) factor dssms are base (generating candidates)");

    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');
    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);

    auto client = NYT::CreateClient(serverProxy);
    client->Create(outputTable, NYT::NT_TABLE, NYT::TCreateOptions().Recursive(true).IgnoreExisting(true));

    const bool isInputSorted = client->Get(inputTable + "/@sorted").AsBool();
    if (!isInputSorted) {
        client->Sort(
            NYT::TSortOperationSpec()
                .AddInput(inputTable)
                .Output(outputTable)
                .SortBy({ "query_id" }));
        inputTable = outputTable;
    }

    // extra memory for initialization of largest model
    memoryLimit += maxModelSize;
    reducerSpec.MemoryLimit(memoryLimit);
    reducerSpec.AddLocalFile(rusListerMapFilename);

    client->Reduce(
        NYT::TReduceOperationSpec()
            .AddInput<NYT::TNode>(inputTable)
            .AddOutput<NYT::TNode>(NYT::TRichYPath(outputTable).SortedBy({ "query_id" }))
            .ReduceBy({ "query_id" })
            .JobCount(jobCount)
            .ReducerSpec(reducerSpec),
        new TCalcFactorsReducer(modelFilename, factorDssmFilenames, numBaseDssmIndexes, rusListerMapFilename, lang));

    return 0;
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    return main_calc_factors(argc, argv);
}
