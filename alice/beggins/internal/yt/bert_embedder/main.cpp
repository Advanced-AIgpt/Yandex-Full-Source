#include <alice/beggins/internal/yt/bert_embedder/config.pb.h>

#include <alice/beggins/internal/bert/embedder.h>

#include <library/cpp/getoptpb/getoptpb.h>
#include <library/cpp/type_info/type_constructors.h>
#include <library/cpp/yson/node/node_io.h>

#include <mapreduce/yt/common/config.h>
#include <mapreduce/yt/interface/client.h>

#include <util/folder/path.h>
#include <util/generic/size_literals.h>
#include <util/system/user.h>

#include <utility>

using namespace NAlice::NBeggins::NInternal;

class TBertEmbedderMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    Y_SAVELOAD_JOB(InputQueryColumnName, OutputEmbeddingsColumnName);
    TBertEmbedderMapper() = default;
    TBertEmbedderMapper(TString inputQueryColumnName, TString outputEmbeddingsColumnName)
        : InputQueryColumnName(std::move(inputQueryColumnName))
        , OutputEmbeddingsColumnName(std::move(outputEmbeddingsColumnName)) {
    }

    void Start(TWriter* /*writer*/) override {
        Embedder = LoadEmbedder(TBlob::FromFileContent(GetModelPath()));
    }

    void Do(TReader* reader, TWriter* writer) override {
        Y_ENSURE(Embedder, "Embedder must not be null");
        for (auto& cursor : *reader) {
            auto outRow = cursor.GetRow();
            outRow[OutputEmbeddingsColumnName] = NYT::TNode::CreateList();
            auto& list = outRow[OutputEmbeddingsColumnName].AsList();
            const auto& result = Embedder->Process(outRow[InputQueryColumnName].AsString());
            for (auto element : TBertEmbedder::ExtractEmbeddings(result)) {
                list.push_back(element);
            }
            writer->AddRow(outRow);
        }
    }

    static const TString& GetModelPath() {
        static const TString modelPath = "model.bert.htxt";
        return modelPath;
    }

private:
    std::unique_ptr<TBertEmbedder> Embedder;
    TString InputQueryColumnName;
    TString OutputEmbeddingsColumnName;
};
REGISTER_MAPPER(TBertEmbedderMapper);

NYT::TTableSchema MakeOutputTableScheme(const NYT::TNode& baseSchema, const TString& outputEmbeddingsColumnName) {
    NYT::TTableSchema outputTableSchema;
    NYT::Deserialize(outputTableSchema, baseSchema);
    outputTableSchema.AddColumn(NYT::TColumnSchema().Name(outputEmbeddingsColumnName).Type(NTi::List(NTi::Double())));
    return outputTableSchema;
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);
    TConfig config = NGetoptPb::GetoptPbOrAbort(argc, argv);
    auto client = NYT::CreateClient(config.GetYtProxy());
    const auto schema = client->Get(config.GetInputTable() + "/@schema");
    auto mapperUserJobSpec = NYT::TUserJobSpec().AddLocalFile(
        config.GetModelPath(), NYT::TAddLocalFileOptions().PathInJob(TBertEmbedderMapper::GetModelPath()));
    if (config.GetMapperJobParams().HasMemoryLimitMb()) {
        mapperUserJobSpec.MemoryLimit(config.GetMapperJobParams().GetMemoryLimitMb() * 1_MB);
    } else {
        mapperUserJobSpec.MemoryLimit(3_GB);
    }
    auto mapperJobSpec =
        NYT::TMapOperationSpec()
            .MaxFailedJobCount(1)
            .MapperSpec(std::move(mapperUserJobSpec))
            .AddInput<NYT::TNode>(config.GetInputTable())
            .AddOutput<NYT::TNode>(NYT::TRichYPath(config.GetOutputTable())
                                       .Schema(MakeOutputTableScheme(schema, config.GetOutputEmbeddingsColumnName())));
    if (config.GetMapperJobParams().HasDataSizePerJobMb()) {
        mapperJobSpec.DataSizePerJob(config.GetMapperJobParams().GetDataSizePerJobMb() * 1_MB);
    }
    client->Map(std::move(mapperJobSpec), MakeIntrusive<TBertEmbedderMapper>(config.GetInputQueryColumnName(),
                                                                             config.GetOutputEmbeddingsColumnName()));
    return 0;
}
