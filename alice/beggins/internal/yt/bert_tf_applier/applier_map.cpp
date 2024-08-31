#include <alice/beggins/internal/bert_tf/applier.h>

#include <alice/beggins/internal/yt/bert_tf_applier/config.pb.h>

#include <library/cpp/getoptpb/getoptpb.h>

#include <library/cpp/type_info/type_constructors.h>

#include <mapreduce/yt/interface/client.h>

#include <util/folder/path.h>
#include <util/generic/size_literals.h>
#include <util/generic/xrange.h>

using namespace NYT;
using namespace NAlice::NBeggins::NBertTfApplier;

class TBertTfApplierMap : public IMapper<TTableReader<TNode>, TTableWriter<TNode>> {
public:
    TBertTfApplierMap(const TConfig& config)
        : Config(config) {
    }

    void Start(TWriter* writer) override {
        Y_UNUSED(writer);
        Applier = std::make_unique<TApplier>(
            std::make_unique<TTokenizer>(TTrie::FromFile(Config.GetStartTriePath()),
                                         TTrie::FromFile(Config.GetContinuationTriePath())),
            TBertDict::FromFile(Config.GetVocabFilePath()),
            TApplier::TTfGraphConfig{.GraphDefPath = Config.GetGraphDefPath(),
                                     .SequenceLength = Config.GetSequenceLength(),
                                     .TokenEmbeddingsOutput = Config.GetTokenEmbeddingsOutput(),
                                     .SentenceEmbeddingOutput = Config.GetSentenceEmbeddingOutput()},
            TApplier::TTfSessionConfig{.NumInterOpThreads = Config.GetNumInterOpThreads(),
                                       .NumIntraOpThreads = Config.GetNumIntraOpThreads(),
                                       .CudaDeviceIdx = Config.GetCudaDeviceIdx()});
    }

    void Do(TReader* inputReader, TWriter* outputWriter) override {
        TVector<TUtf32String> batch;
        TVector<TNode> passThroughBatch;
        size_t batchSize = Config.GetBatchSize();
        const auto& inputTextColumn = Config.GetInputTextColumn();
        for (auto& cursor : *inputReader) {
            const auto& row = cursor.GetRow();
            batch.push_back(TUtf32String::FromUtf8(row[inputTextColumn].AsString()));
            passThroughBatch.push_back(row);
            if (batch.size() >= batchSize) {
                DumpBatch(outputWriter, batch, passThroughBatch);
            }
        }
        if (!batch.empty()) {
            DumpBatch(outputWriter, batch, passThroughBatch);
        }
    }

    TBertTfApplierMap() = default;

    Y_SAVELOAD_JOB(Config);

private:
    void DumpBatch(TWriter* outputWriter, TVector<TUtf32String>& batch, TVector<TNode>& passThroughBatch) {
        auto applierResult = Applier->Apply(batch);

        for (auto sampleIdx : xrange(batch.size())) {
            auto outRow = passThroughBatch[sampleIdx];

            if (Config.HasOutputSentenceEmbeddingColumn()) {
                const auto& column = Config.GetOutputSentenceEmbeddingColumn();
                outRow[column] = TNode::CreateList();
                auto& sentenceEmbedding = outRow[column].AsList();
                for (auto sentenceFeature : applierResult[sampleIdx].SentenceEmbedding) {
                    sentenceEmbedding.push_back(sentenceFeature);
                }
            }

            if (Config.HasOutputWordsColumn()) {
                const auto& column = Config.GetOutputWordsColumn();
                outRow[column] = TNode::CreateList();
                auto& words = outRow[column].AsList();
                for (const auto& word : applierResult[sampleIdx].Words) {
                    words.push_back(WideToUTF8(word));
                }
            }

            if (Config.HasOutputWordEmbeddingsColumn()) {
                const auto& column = Config.GetOutputWordEmbeddingsColumn();
                outRow[column] = TNode::CreateList();
                auto& wordEmbeddings = outRow[column].AsList();
                for (const auto& wordEmbedding : applierResult[sampleIdx].WordEmbeddings) {
                    auto embeddingNode = TNode::CreateList();
                    auto& embedding = embeddingNode.AsList();
                    for (const auto wordFeature : wordEmbedding) {
                        embedding.push_back(wordFeature);
                    }
                    wordEmbeddings.push_back(embeddingNode);
                }
            }

            outputWriter->AddRow(outRow);
        }

        batch.clear();
        passThroughBatch.clear();
    }

private:
    std::unique_ptr<TApplier> Applier;
    TConfig Config;
};

REGISTER_MAPPER(TBertTfApplierMap);

int main(int argc, const char** argv) {
    Initialize(argc, argv);
    TConfig config = NGetoptPb::GetoptPbOrAbort(argc, argv);
    TFsPath startTriePath(config.GetStartTriePath());
    config.SetStartTriePath(startTriePath.GetName());
    TFsPath continuationTriePath(config.GetContinuationTriePath());
    config.SetContinuationTriePath(continuationTriePath.GetName());
    TFsPath vocabFilePath(config.GetVocabFilePath());
    config.SetVocabFilePath(vocabFilePath.GetName());
    TFsPath graphDefPath(config.GetGraphDefPath());
    config.SetGraphDefPath(graphDefPath.GetName());

    auto client = CreateClient(config.GetYtProxy());
    TTableSchema schema;
    Deserialize(schema, client->Get(config.GetInputTable() + "/@schema"));

    if (config.HasOutputSentenceEmbeddingColumn()) {
        schema.AddColumn(
            TColumnSchema().Name(config.GetOutputSentenceEmbeddingColumn()).Type(NTi::List(NTi::Float())));
    }

    if (config.HasOutputWordEmbeddingsColumn()) {
        schema.AddColumn(
            TColumnSchema().Name(config.GetOutputWordEmbeddingsColumn()).Type(NTi::List(NTi::List(NTi::Float()))));
    }

    if (config.HasOutputWordsColumn()) {
        schema.AddColumn(TColumnSchema().Name(config.GetOutputWordsColumn()).Type(NTi::List(NTi::String())));
    }

    if (!config.HasBatchSize()) {
        config.SetBatchSize(512);
    }

    const ui64 dataSizePerJob = config.HasDataSizePerJobKB() ? 1_KB * config.GetDataSizePerJobKB() : 512_KB;
    const ui64 memoryLimit = config.HasMemoryLimitMB() ? 1_MB * config.GetMemoryLimitMB() : 50_GB;

    Cerr << "Bert Applier Config passed to job: " << config << Endl;

    client->Map(TMapOperationSpec()
                    .AddInput<TNode>(config.GetInputTable())
                    .AddOutput<TNode>(TRichYPath(config.GetOutputTable()).Schema(schema))
                    .MapperSpec(TUserJobSpec()
                                    .AddLocalFile(startTriePath.GetPath())
                                    .AddLocalFile(continuationTriePath.GetPath())
                                    .AddLocalFile(vocabFilePath.GetPath())
                                    .AddLocalFile(graphDefPath.GetPath())
                                    .MemoryLimit(memoryLimit))
                    .DataSizePerJob(dataSizePerJob),
                MakeIntrusive<TBertTfApplierMap>(config));

    Cout << "Done " << config << Endl;
}
