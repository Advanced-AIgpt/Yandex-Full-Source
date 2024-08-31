#include <alice/boltalka/libs/text_utils/utterance_transform.h>

#include <dict/mt/libs/punctuation/punctuator.h>
#include <dict/mt/libs/punctuation/neural_punctuator.h>

#include <library/cpp/getopt/last_getopt.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/operation.h>

#include <util/charset/wide.h>
#include <util/folder/path.h>
#include <util/system/fstat.h>

class TApplyPunctMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TApplyPunctMapper() = default;

    explicit TApplyPunctMapper(const TString& configFilename)
        : ConfigFilename(configFilename)
    {
    }

    void Start(NYT::TTableWriter<NYT::TNode>*) override {
        Punctuator.Reset(NDict::NMT::MakeNeuralPunctuator(ConfigFilename).release());
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();
            TString replyKey = "reply";
            if (row.HasKey("rewritten_reply")) {
                replyKey = "rewritten_reply";
            }
            TString reply = NNlgTextUtils::TSeparatePunctuation().Transform(row[replyKey].AsString());
            const auto punctuatedReply = Punctuator->Punctuate(UTF8ToWide(reply));
            NYT::TNode result = row;
            result["punctuated_reply"] = WideToUTF8(punctuatedReply);
            output->AddRow(result);
        }
    }

    Y_SAVELOAD_JOB(ConfigFilename);

private:
    TString ConfigFilename;
    THolder<NDict::NMT::TPunctuator> Punctuator;
};
REGISTER_MAPPER(TApplyPunctMapper);

int main_apply_punct(int argc, const char** argv) {
    TString serverProxy;
    TString inputTable;
    TString outputTable;
    TString configFilename;
    ui32 jobCount;
    NYT::TUserJobSpec userJobSpec;
    ui64 memoryLimit = 1ULL << 29;

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
        .AddLongOption('c', "config")
        .RequiredArgument("FILE")
        .Required()
        .StoreResult(&configFilename)
        .Help("Punctuation config file.");
    opts
        .AddLongOption('f', "file")
        .RequiredArgument("FILE")
        .Required()
        .Handler1T<TString>([&userJobSpec, &memoryLimit](const TString& arg) {
            userJobSpec.AddLocalFile(arg);
            memoryLimit += GetFileLength(arg) * 3 / 2;
        })
        .Help("Add additional files (model and vocabularies) by repeating this option");
    opts
        .AddLongOption('j', "job-count")
        .DefaultValue("300")
        .StoreResult(&jobCount);

    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');
    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);

    auto client = NYT::CreateClient(serverProxy);
    client->Create(outputTable, NYT::NT_TABLE, NYT::TCreateOptions().Recursive(true).IgnoreExisting(true));

    userJobSpec.AddLocalFile(configFilename);
    userJobSpec.MemoryLimit(memoryLimit);

    client->Map(
        NYT::TMapOperationSpec()
            .AddInput<NYT::TNode>(inputTable)
            .AddOutput<NYT::TNode>(outputTable)
            .MapperSpec(userJobSpec)
            .JobCount(jobCount),
        new TApplyPunctMapper(TFsPath(configFilename).Basename()));

    return 0;
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    return main_apply_punct(argc, argv);
}
