#include "index_type.h"

#include <alice/boltalka/libs/text_utils/context_transform.h>
#include <alice/boltalka/libs/dssm_model/dssm_model.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/dot_product/dot_product.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/operation.h>
#include <mapreduce/yt/util/temp_table.h>

#include <contrib/libs/intel/mkl/include/mkl.h>

#include <util/digest/city.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/system/fstat.h>

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

class TApplyDssmMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TApplyDssmMapper() = default;

    TApplyDssmMapper(const TVector<TString>& dssmModelsFilenames,
                     const TVector<TString>& outputColumnPrefixes,
                     size_t batchSize,
                     ELanguage lang,
                     TString contextColumnPrefix,
                     TString replyColumn)
        : OutputColumnPrefixes(outputColumnPrefixes)
        , BatchSize(batchSize)
        , Lang(lang)
        , ContextColumnPrefix(contextColumnPrefix)
        , ReplyColumn(replyColumn)
    {
        for (const auto& filename : dssmModelsFilenames) {
            DssmModelsFilenames.push_back(TFsPath(filename).Basename());
        }
    }

    void Start(NYT::TTableWriter<NYT::TNode>*) override {
        mkl_set_num_threads(1);
        mkl_cbwr_set(MKL_CBWR_COMPATIBLE);

        for (const auto& modelName : DssmModelsFilenames) {
            DssmModels.push_back(new NNlg::TDssmModel(modelName));
        }

        ContextTransform = new NNlgTextUtils::TNlgSearchContextTransform(Lang);
        ReplyTransform = new NNlgTextUtils::TNlgSearchUtteranceTransform(Lang);
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        TVector<NYT::TNode> batch;
        batch.reserve(BatchSize);
        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();
            batch.push_back(row);
            if (batch.size() == BatchSize) {
                DoBatch(&batch, output);
                batch.clear();
            }
        }
        if (!batch.empty()) {
            DoBatch(&batch, output);
        }
    }

    void DoBatch(TVector<NYT::TNode>* batch, NYT::TTableWriter<NYT::TNode>* output) {
        TVector<TVector<TString>> contexts;
        TVector<TString> replies;
        for (const auto& row : *batch) {
            auto context = ContextTransform->Transform(ParseByPrefix<TString>(row, ContextColumnPrefix));
            auto reply = ReplyTransform->Transform(row[ReplyColumn].AsString());
            contexts.push_back(context);
            replies.push_back(reply);
        }
        for (size_t i = 0; i < DssmModels.size(); ++i) {
            AddEmbeddings(DssmModels[i], contexts, replies, OutputColumnPrefixes[i], batch);
        }
        for (const auto& row : *batch) {
            output->AddRow(row);
        }
    }

    Y_SAVELOAD_JOB(DssmModelsFilenames, OutputColumnPrefixes, BatchSize, Lang, ContextColumnPrefix, ReplyColumn);

private:
    static void AddEmbeddings(const NNlg::TDssmModelPtr& dssmModel,
                              const TVector<TVector<TString>>& contexts,
                              const TVector<TString>& replies,
                              const TString& columnPrefix,
                              TVector<NYT::TNode>* rows) {
        TVector<TVector<float>> embeddings = dssmModel->FpropBatch(contexts, replies, { "query_embedding", "doc_embedding" });
        Y_VERIFY(embeddings[0].size() == embeddings[1].size());
        const float* contextEmbeddings = embeddings[0].data();
        const float* replyEmbeddings = embeddings[1].data();
        const size_t dimension = embeddings[0].size() / rows->size();
        for (auto& row : *rows) {
            row[columnPrefix + "context_embedding"] = EmbeddingToString(contextEmbeddings, dimension);
            row[columnPrefix + "reply_embedding"] = EmbeddingToString(replyEmbeddings, dimension);
            row[columnPrefix + "dot_product"] = DotProduct(contextEmbeddings, replyEmbeddings, dimension);
            contextEmbeddings += dimension;
            replyEmbeddings += dimension;
        }
    }

    static TString EmbeddingToString(const float* embedding, size_t dimension) {
        return {reinterpret_cast<const char*>(embedding), dimension * sizeof(float)};
    }

private:
    TVector<TString> DssmModelsFilenames;
    TVector<TString> OutputColumnPrefixes;
    size_t BatchSize;
    ELanguage Lang;
    TString ContextColumnPrefix;
    TString ReplyColumn;
    TVector<NNlg::TDssmModelPtr> DssmModels;
    NNlgTextUtils::IContextTransformPtr ContextTransform;
    NNlgTextUtils::IUtteranceTransformPtr ReplyTransform;
};
REGISTER_MAPPER(TApplyDssmMapper);

int main_apply_dssm(int argc, const char** argv) {
    TString serverProxy;
    TString inputTable;
    TString outputTable;
    TString modelFilename;
    ui32 jobCount;
    size_t batchSize;
    ELanguage lang;
    TString contextColumnPrefix;
    TString replyColumn;

    ui64 memoryLimit = (2ULL << 31);
    ui64 maxModelSize = 0;
    auto mapperSpec = NYT::TUserJobSpec();

    auto addModelHandler = [&](const TString& filename) {
        mapperSpec.AddLocalFile(filename);
        ui64 modelSize = GetFileLength(filename);
        maxModelSize = Max(maxModelSize, modelSize);
        memoryLimit += modelSize;
    };

    TVector<TString> dssmModelsFilenames;
    TVector<TString> outputColumnPrefixes;

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
        .AddLongOption("context-column-prefix")
        .RequiredArgument("STRING")
        .DefaultValue("context_")
        .StoreResult(&contextColumnPrefix);
    opts
        .AddLongOption("reply-column")
        .RequiredArgument("STRING")
        .DefaultValue("reply")
        .StoreResult(&replyColumn);
    opts
        .AddLongOption('m', "model")
        .RequiredArgument("FILE")
        .Handler1T<TString>([&](const TString& filename) {
            addModelHandler(filename);
            dssmModelsFilenames.push_back(filename);
            outputColumnPrefixes.push_back("");
        })
        .Help("Dssm model file.");
    opts
        .AddLongOption('j', "job-count")
        .RequiredArgument("INT")
        .DefaultValue("200")
        .StoreResult(&jobCount);
    opts
        .AddLongOption('b', "batch-size")
        .RequiredArgument("INT")
        .DefaultValue("100")
        .StoreResult(&batchSize);
    opts.
        AddLongOption("factor-dssm-0")
        .RequiredArgument("FILE")
        .Handler1T<TString>([&](const TString& filename) {
            addModelHandler(filename);
            dssmModelsFilenames.push_back(filename);
            outputColumnPrefixes.push_back("factor_dssm_0:");
        })
        .Help("Factor dssm model file.");
    opts.
        AddLongOption("factor-dssm-1")
        .RequiredArgument("FILE")
        .Handler1T<TString>([&](const TString& filename) {
            addModelHandler(filename);
            dssmModelsFilenames.push_back(filename);
            outputColumnPrefixes.push_back("factor_dssm_1:");
        })
        .Help("Factor dssm model file.");
    opts.
        AddLongOption("factor-dssm-2")
        .RequiredArgument("FILE")
        .Handler1T<TString>([&](const TString& filename) {
            addModelHandler(filename);
            dssmModelsFilenames.push_back(filename);
            outputColumnPrefixes.push_back("factor_dssm_2:");
        })
        .Help("Factor dssm model file.");
    opts.
        AddLongOption("factor-dssm-3")
        .RequiredArgument("FILE")
        .Handler1T<TString>([&](const TString& filename) {
            addModelHandler(filename);
            dssmModelsFilenames.push_back(filename);
            outputColumnPrefixes.push_back("factor_dssm_3:");
        })
        .Help("Factor dssm model file.");
    opts.
        AddLongOption("factor-dssm-4")
        .RequiredArgument("FILE")
        .Handler1T<TString>([&](const TString& filename) {
            addModelHandler(filename);
            dssmModelsFilenames.push_back(filename);
            outputColumnPrefixes.push_back("factor_dssm_4:");
        })
        .Help("Factor dssm model file.");
    opts.
        AddLongOption("factor-dssm-5")
        .RequiredArgument("FILE")
        .Handler1T<TString>([&](const TString& filename) {
            addModelHandler(filename);
            dssmModelsFilenames.push_back(filename);
            outputColumnPrefixes.push_back("factor_dssm_5:");
        })
        .Help("Factor dssm model file.");

    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');
    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);

    Y_VERIFY(dssmModelsFilenames.size() > 0);
    Y_VERIFY(dssmModelsFilenames.size() == outputColumnPrefixes.size());

    // extra memory for initialization of largest model
    memoryLimit += maxModelSize;
    mapperSpec.MemoryLimit(memoryLimit);

    auto client = NYT::CreateClient(serverProxy);
    client->Create(outputTable, NYT::NT_TABLE, NYT::TCreateOptions().Recursive(true).IgnoreExisting(true));

    client->Map(
        NYT::TMapOperationSpec()
            .AddInput<NYT::TNode>(inputTable)
            .AddOutput<NYT::TNode>(outputTable)
            .MapperSpec(mapperSpec)
            .JobCount(jobCount),
        new TApplyDssmMapper(dssmModelsFilenames, outputColumnPrefixes, batchSize, lang, contextColumnPrefix, replyColumn));

    return 0;
}

class TAssignHashCodesMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TAssignHashCodesMapper() = default;

    TAssignHashCodesMapper(const TString& shardColumn)
        : ShardColumn(shardColumn)
    {
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();
            const auto idPostfix = GetPostfix(row);
            TString context = JoinSeq("\t", ParseByPrefix<TString>(row, "context_"));
            TString reply = row["reply"].AsString();
            NYT::TNode result = row;
            result["hash_code"] = CityHash64(reply + "\t" + context + idPostfix);
            result["context_hash_code"] = CityHash64(context + idPostfix);
            result["reply_hash_code"] = CityHash64(reply + idPostfix);
            output->AddRow(result);
        }
    }

    Y_SAVELOAD_JOB(ShardColumn);

private:
    TString GetPostfix(const NYT::TNode& row) const {
        if (ShardColumn.empty()) {
            return "";
        } else {
            return "\t" + row[ShardColumn].AsString();
        }
    }

private:
    TString ShardColumn;
};
REGISTER_MAPPER(TAssignHashCodesMapper);

class TAssignDocAndShardIdMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TAssignDocAndShardIdMapper() = default;

    TAssignDocAndShardIdMapper(size_t shardSize)
        : ShardSize(shardSize)
    {
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();
            NYT::TNode result = row;
            result["shard_id"] = input->GetRowIndex() / ShardSize;
            result["doc_id"] = input->GetRowIndex() % ShardSize;
            output->AddRow(result);
        }
    }

    Y_SAVELOAD_JOB(ShardSize);

private:
    size_t ShardSize;
};
REGISTER_MAPPER(TAssignDocAndShardIdMapper);


class TAssignDocAndPrecomputedShardIdReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TAssignDocAndPrecomputedShardIdReducer() = default;

    TAssignDocAndPrecomputedShardIdReducer(const THashMap<TString, size_t>& shardIdEncoding, const TString& shardColumn)
        : ShardIdEncoding(shardIdEncoding)
        , ShardColumn(shardColumn)
    {
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        size_t i = 0;
        if (!ShardIdEncoding.FindPtr(input->GetRow()[ShardColumn].AsString())) {
            return;
        }
        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();
            NYT::TNode result = row;
            Cerr << row[ShardColumn].AsString() << Endl;

            result["shard_id"] = ShardIdEncoding[row[ShardColumn].AsString()];
            result["doc_id"] = i;
            ++i;
            output->AddRow(result);
        }
    }

    Y_SAVELOAD_JOB(ShardIdEncoding, ShardColumn);

private:
    THashMap<TString, size_t> ShardIdEncoding;
    TString ShardColumn;
};
REGISTER_REDUCER(TAssignDocAndPrecomputedShardIdReducer);

class TUniqueReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        Y_VERIFY(input->IsValid());
        output->AddRow(input->GetRow());
    }
};
REGISTER_REDUCER(TUniqueReducer);

void AssignShardAndDocId(NYT::IClientPtr client, const TString& mainOutputTable, size_t numShards) {
    client->Sort(
        NYT::TSortOperationSpec()
            .AddInput(mainOutputTable)
            .Output(mainOutputTable)
            .SortBy({ "hash_code" }));
    const size_t numDocs = client->Get(mainOutputTable + "/@row_count").AsInt64();
    const size_t shardSize = (numDocs + numShards - 1) / numShards;
    client->Map(
        NYT::TMapOperationSpec()
            .AddInput<NYT::TNode>(mainOutputTable)
            .AddOutput<NYT::TNode>(NYT::TRichYPath(mainOutputTable).SortedBy({ "shard_id", "doc_id", "hash_code" }))
            .Ordered(true),
        new TAssignDocAndShardIdMapper(shardSize));
}

void AssignDocIdFromPreassignedShards(NYT::IClientPtr client, const TString& mainOutputTable, const TString& preassignedShardColumn) {
    client->Sort(
        NYT::TSortOperationSpec()
            .AddInput(mainOutputTable)
            .Output(mainOutputTable)
            .SortBy({preassignedShardColumn, "hash_code" })
    );

    NYT::TTempTable tmpTable(client);
    client->Reduce(
        NYT::TReduceOperationSpec()
            .AddInput<NYT::TNode>(mainOutputTable)
            .AddOutput<NYT::TNode>(tmpTable.Name())
            .ReduceBy({ preassignedShardColumn }),
        new TUniqueReducer);
    THashMap<TString, size_t> shardIdEncoding;
    auto reader = client->CreateTableReader<NYT::TNode>(tmpTable.Name());
    for (; reader->IsValid(); reader->Next()) {
        shardIdEncoding.try_emplace(reader->GetRow()[preassignedShardColumn].AsString(), shardIdEncoding.size());
    }
    client->Reduce(
        NYT::TReduceOperationSpec()
            .AddInput<NYT::TNode>(mainOutputTable)
            .AddOutput<NYT::TNode>(NYT::TRichYPath(mainOutputTable))
            .ReduceBy({preassignedShardColumn}),
        new TAssignDocAndPrecomputedShardIdReducer(shardIdEncoding, preassignedShardColumn));
    client->Sort(
        NYT::TSortOperationSpec()
            .AddInput(mainOutputTable)
            .Output(mainOutputTable)
            .SortBy({"shard_id", "doc_id", "hash_code" })
    );

}

int main_assign_shards(int argc, const char** argv) {
    TString inputTable;
    TString serverProxy;
    TString outputDir;
    size_t numShards = 0;
    TString shardColumn;

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
        .AddLongOption('o', "output-dir")
        .RequiredArgument("DIR")
        .Required()
        .StoreResult(&outputDir);
    opts
        .AddLongOption('n', "num-shards")
        .RequiredArgument("INT")
        .StoreResult(&numShards);
    opts
        .AddLongOption('s', "shard-column")
        .StoreResult(&shardColumn)
        .Help("Precomputed shard id");
    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');
    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);
    Y_ENSURE((numShards > 0) ^ (!shardColumn.empty()));

    auto client = NYT::CreateClient(serverProxy);

    const TString mainOutputTable = outputDir + "/" + ToString(EIndexType::ContextAndReply);
    client->Create(mainOutputTable, NYT::NT_TABLE, NYT::TCreateOptions().Recursive(true).IgnoreExisting(true));

    client->MapReduce(
        NYT::TMapReduceOperationSpec()
            .AddInput<NYT::TNode>(inputTable)
            .AddOutput<NYT::TNode>(mainOutputTable)
            .SortBy({ "hash_code" })
            .ReduceBy({ "hash_code" }),
        new TAssignHashCodesMapper(shardColumn),
        new TUniqueReducer);
    if (shardColumn.empty()) {
        AssignShardAndDocId(client, mainOutputTable, numShards);
    }  else {
        AssignDocIdFromPreassignedShards(client, mainOutputTable, shardColumn);
    }

    for (EIndexType indexType : GetEnumAllValues<EIndexType>()) {
        TString indexName = ToString(indexType);
        TString output = outputDir + "/" + indexName;
        if (output != mainOutputTable) {
            client->Sort(
                NYT::TSortOperationSpec()
                    .AddInput(mainOutputTable)
                    .Output(output)
                    .SortBy({ indexName + "_hash_code" }));
            client->Reduce(
                NYT::TReduceOperationSpec()
                    .AddInput<NYT::TNode>(output)
                    .AddOutput<NYT::TNode>(output)
                    .ReduceBy({ indexName + "_hash_code" }),
                new TUniqueReducer);
        }
        client->Sort(
            NYT::TSortOperationSpec()
                .AddInput(output)
                .Output(output)
                .SortBy({ "shard_id", "doc_id" }));
    }

    return 0;
}

int main_download_shard(int argc, const char** argv) {
    TVector<TString> inputDirs;
    TFsPath outputDir;
    TString serverProxy;
    TString indexTypesStr;
    size_t shardId;
    const TString baseDssmName = "base_dssm";

    TString allIndexTypesStr;
    for (auto indexType : GetEnumAllValues<EIndexType>()) {
        if (!allIndexTypesStr.empty()) {
            allIndexTypesStr += ',';
        }
        allIndexTypesStr += ToString(indexType);
    }

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
        .AddLongOption('i', "input-dir")
        .RequiredArgument("DIR")
        .Handler1T<TString>([&](const TString& path) {
            inputDirs.push_back(path);
        });
    opts
        .AddLongOption('o', "output-dir")
        .RequiredArgument("DIR")
        .Required()
        .StoreResult(&outputDir);
    opts
        .AddLongOption('s', "shard-id")
        .RequiredArgument("INT")
        .Required()
        .StoreResult(&shardId);
    opts
        .AddLongOption('t', "index-types")
        .RequiredArgument("STRING")
        .DefaultValue(allIndexTypesStr)
        .StoreResult(&indexTypesStr)
        .Help("Comma-separated list of index types to download.");

    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);
    Y_VERIFY(inputDirs.size() > 0);

    outputDir.MkDirs();

    auto client = NYT::CreateClient(serverProxy);
    for (auto split : StringSplitter(indexTypesStr).Split(',')) {
        TString indexTypeStr = TString{split.Token()};
        auto indexType = FromString<EIndexType>(indexTypeStr);
        THolder<TFileOutput> textOut;
        THolder<TFileOutput> staticFactorsOut;
        if (indexType == EIndexType::ContextAndReply) {
            textOut = MakeHolder<TFileOutput>(outputDir / "context_and_reply.txt");
            staticFactorsOut = MakeHolder<TFileOutput>(outputDir / "static_factors.bin");
        }
        TVector<TString> vectorFields;
        if (indexType == EIndexType::Context || indexType == EIndexType::ContextAndReply) {
            vectorFields.push_back("context_embedding");
        }
        if (indexType == EIndexType::Reply || indexType == EIndexType::ContextAndReply) {
            vectorFields.push_back("reply_embedding");
        }
        ui32 docIdShift = 0;
        for (const auto& inputDir : inputDirs) {
            Y_VERIFY(client->Exists(inputDir + "/" + ToString(EIndexType::ContextAndReply)));
            TString input = inputDir + "/" + indexTypeStr;
            if (!client->Exists(input)) {
                continue;
            }
            const auto indexName = StringSplitter(inputDir).Split('/').ToList<TString>().back();
            const auto indexDir = outputDir / baseDssmName / indexName;
            indexDir.MkDirs();
            TString vecOutPath = indexDir / (indexTypeStr + ".vec");
            TString idsOutPath = indexDir / (indexTypeStr + ".ids");
            TFileOutput vecOut(vecOutPath);
            TFileOutput idsOut(idsOutPath);
            TVector<THolder<TFileOutput>> factorDssmVectorsOut;
            TVector<THolder<TFileOutput>> factorDssmIdsOut;
            auto reader = client->CreateTableReader<NYT::TNode>(
                NYT::TRichYPath(input).AddRange(NYT::TReadRange().Exact(NYT::TReadLimit().Key(shardId))));

            for (; reader->IsValid(); reader->Next()) {
                const auto& row = reader->GetRow();
                for (const auto& field : vectorFields) {
                    auto vectorStr = row[field].AsString();
                    vecOut.Write(vectorStr.data(), vectorStr.size());
                }
                ui32 id = row["doc_id"].AsUint64() + docIdShift;
                idsOut.Write(&id, sizeof(id));
                if (textOut) {
                    auto context = ParseByPrefix<TString>(row, "context_");
                    std::reverse(context.begin(), context.end());
                    TString reply = row.HasKey("rewritten_reply") ? row["rewritten_reply"].AsString() : row["reply"].AsString();
                    //TString disrespect_reply = row.HasKey("disrespect_reply") ? row["disrespect_reply"].AsString() : "";
                    TString disrespect_reply = "";
                    TString source = row.HasKey("source") ? row["source"].AsString() : "";
                    *textOut << JoinSeq("\t", context) << '\t' << reply << '\t' << disrespect_reply << '\t' << source << '\n';
                }
                if (staticFactorsOut) {
                    auto staticFactors = ParseByPrefix<double, float>(row, "static_factor_");
                    staticFactorsOut->Write(staticFactors.data(), staticFactors.size() * sizeof(float));
                }
                if (indexType == EIndexType::ContextAndReply) {
                    for (size_t i = 0; ; ++i) {
                        const TString factorDssmModelName = "factor_dssm_" + ToString(i);
                        if (!row.HasKey(factorDssmModelName + ":" + vectorFields[0])) {
                            break;
                        }
                        if (i == factorDssmVectorsOut.size()) {
                            const auto indexDir = outputDir / factorDssmModelName / indexName;
                            indexDir.MkDirs();
                            factorDssmVectorsOut.emplace_back(new TFileOutput(indexDir / (indexTypeStr + ".vec")));
                            factorDssmIdsOut.emplace_back(new TFileOutput(indexDir / (indexTypeStr + ".ids")));
                        }
                        for (const auto& field : vectorFields) {
                            auto vectorStr = row[factorDssmModelName + ":" + field].AsString();
                            factorDssmVectorsOut[i]->Write(vectorStr.data(), vectorStr.size());
                        }
                        factorDssmIdsOut[i]->Write(&id, sizeof(id));
                    }
                }
            }
            docIdShift += client->Get(inputDir + "/" + ToString(EIndexType::ContextAndReply) + "/@row_count").AsInt64();
        }
    }

    auto staticFactorsFile = outputDir / "static_factors.bin";
    if (GetFileLength(staticFactorsFile) == 0) {
        staticFactorsFile.ForceDelete();
    }

    return 0;
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    TModChooser modChooser;

    modChooser.AddMode(
        "apply-dssm",
        main_apply_dssm,
        "-- apply dssm");

    modChooser.AddMode(
        "assign-shards",
        main_assign_shards,
        "-- assign shards");

    modChooser.AddMode(
        "download-shard",
        main_download_shard,
        "-- download shard");

    return modChooser.Run(argc, argv);
}
