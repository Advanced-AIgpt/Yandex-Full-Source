#include <kernel/factor_storage/factor_storage.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <mapreduce/yt/interface/client.h>

#include <util/generic/guid.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/system/user.h>


using namespace NYT;

std::pair<TString, TString> DecodeFactors(const TString& encoded) {
    TString serialized;
    Base64Decode(encoded, serialized);
    TStringStream serializedSS(serialized);
    const NFactorSlices::TSlicesMetaInfo sliceInfo;
    const THolder<TFactorStorage> storage = NFSSaveLoad::Deserialize(&serializedSS, sliceInfo);

    TString factorBorders = SerializeFactorBorders(*storage);
    TStringBuilder humanReadableFactors;
    for (ui32 i = 0; i < storage->Size(); ++i) {
        humanReadableFactors << (i == 0 ? "" : "\t") << (*storage)[i];
    }

    return {factorBorders, humanReadableFactors};
}

class TDecodeFactorsMapper
    : public IMapper<TTableReader<TNode>, TTableWriter<TNode>>
{
public:
    void Do(TReader* reader, TWriter* writer) override
    {
        int cnt = 0;
        TMap<TString, int> sliceID;

        for (auto& cursor : *reader) {
            const auto& row = cursor.GetRow();
            TNode outRow = row;

            auto [factorBorders, humanReadableFactors] = DecodeFactors(row["factors"].AsString());

            outRow["factors"] = humanReadableFactors;
            outRow["slice"] = factorBorders;

            int id;
            auto it = sliceID.find(factorBorders);
            if (it != sliceID.end()) {
                id = it->second;
            } else {
                id = cnt++;
                sliceID[factorBorders] = id;
            }
            outRow["sliceID"] = id;

            writer->AddRow(outRow);
        }
    }
};
REGISTER_MAPPER(TDecodeFactorsMapper);


int main(int argc, const char** argv) {
    Initialize(argc, argv);

    NLastGetopt::TOpts opts;
    opts.AddHelpOption('h');

    TString inputTable;
    opts.AddLongOption('i', "input", "MR table with encoded factors")
        .Required()
        .RequiredArgument()
        .StoreResult(&inputTable);

    TString outputTable;
    opts.AddLongOption('o', "output", "MR table with decoded factors")
        .Required()
        .RequiredArgument()
        .StoreResult(&outputTable);

    TString token;
    opts.AddLongOption('t', "token", "YT Token")
        .Required()
        .RequiredArgument()
        .StoreResult(&token);

    NLastGetopt::TOptsParseResult parseOpts(&opts, argc, argv);

    auto client = CreateClient("hahn", NYT::TCreateClientOptions().Token(token));

    auto op = client->Map(
        TMapOperationSpec()
            .AddInput<TNode>(inputTable)
            .AddOutput<TNode>(outputTable),
        new TDecodeFactorsMapper,
        TOperationOptions{}.InferOutputSchema(true));

    Cout << "Operation: " << op->GetWebInterfaceUrl() << Endl;
    Cout << "Output table: https://yt.yandex-team.ru/hahn/navigation?path=" << outputTable << Endl;


    return 0;
}
