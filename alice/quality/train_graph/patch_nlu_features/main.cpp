#include <alice/protos/api/nlu/feature_container.pb.h>

#include <kernel/alice/begemot_nlu_factors_info/fill_factors/fill_nlu_factors.h>
#include <kernel/factor_storage/factor_storage.h>
#include <kernel/factor_storage/factors_reader.h>

#include <library/cpp/getopt/last_getopt.h>

#include <mapreduce/yt/interface/client.h>

#include <util/string/split.h>
#include <util/string/join.h>

using namespace NYT;

class TNLUFeaturesPatcher : public IMapper<TTableReader<TNode>, TTableWriter<TNode>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        for (const auto& cursor : *reader) {
            const auto& row = cursor.GetRow();

            TNode outRow = row;

            TVector<float> features;
            StringSplitter(row["features"].AsString()).Split('\t').ParseInto(&features);

            TFactorBorders borders;
            NFactorSlices::DeserializeFactorBorders(row["slice"].AsString(), borders);

            NFSSaveLoad::TFactorsReader reader(borders, NFSSaveLoad::CreateRawFloatsInput(features.begin(), features.size()));
            TFactorStorage storage;
            reader.ReadTo(storage, NFactorSlices::TGlobalSlicesMetaInfo::Instance());

            NAlice::TFeatureContainer begemotFeatures;
            for (const auto& feature : row["begemot_features"].AsList()) {
                begemotFeatures.AddFeatures(feature.AsDouble());
            }

            NAlice::NNluFeatures::FillNluFactorsSlice(begemotFeatures, storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_BEGEMOT_NLU_FACTORS));

            outRow["features"] = JoinSeq("\t", storage.CreateConstView());

            outRow["slice"] = SerializeFactorBorders(storage.GetBorders(), NFactorSlices::ESerializationMode::LeafOnly);

            writer->AddRow(outRow);
        }

    }

    void PrepareOperation(const IOperationPreparationContext& context, TJobOperationPreparer& preparer) const override {
        preparer.OutputSchema(/* tableIndex */ 0, context.GetInputSchema(/* tableIndex */ 0));
    }
};
REGISTER_MAPPER(TNLUFeaturesPatcher);

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    NLastGetopt::TOpts options;
    options.SetFreeArgsNum(0);

    TString proxyName;
    options.AddLongOption('p', "proxy", "YT proxy name")
        .Required()
        .RequiredArgument("PROXY")
        .StoreResult(&proxyName);

    NYT::TYPath inputTable;
    options.AddLongOption('i', "input", "input table")
        .Required()
        .RequiredArgument("YPATH")
        .StoreResult(&inputTable);

    NYT::TYPath outputTable;
    options.AddLongOption('o', "output", "output table")
        .Required()
        .RequiredArgument("YPATH")
        .StoreResult(&outputTable);

    NLastGetopt::TOptsParseResult parseOpts{&options, argc, argv};

    auto client = CreateClient(proxyName);

    client->Map(
        TMapOperationSpec()
            .AddInput<TNode>(inputTable)
            .AddOutput<TNode>(outputTable),
        new TNLUFeaturesPatcher);

    return 0;
}
