#include <alice/nlu/libs/sample_features/sample_features.h>

#include <mapreduce/yt/interface/client.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/iterator/zip.h>
#include <library/cpp/langs/langs.h>

#include <util/stream/output.h>

using namespace NYT;

namespace {

    class TConverter: public IMapper<TTableReader<TNode>, NYT::TTableWriter<TNode>> {
    public:
        Y_SAVELOAD_JOB(Language);

        TConverter() = default;
        explicit TConverter(ELanguage lang)
            : Language(lang)
        {}

        void Do(TReader* reader, TWriter* writer) override {
            Y_ENSURE(reader);
            Y_ENSURE(writer);

            for (const auto& cursor : *reader) {
                const TNode& inputNode = cursor.GetRow();

                TNode outputNode = inputNode;
                outputNode["sample_features"]["sample"] = SampleInfo(inputNode);
                outputNode["sample_features"]["dense_seq_features"] = DenseSeqFeatures(inputNode);
                outputNode["sample_features"]["sparse_seq_features"] = SparseSeqFeaturesStub(
                    outputNode["sample_features"]["sample"]["tokens"].AsList().size());

                writer->AddRow(outputNode);
            }
        }

        void PrepareOperation(const IOperationPreparationContext& context, TJobOperationPreparer& preparer) const override {
            NYT::TTableSchema schema = context.GetInputSchema(/* tableIndex */ 0);
            schema.AddColumn("sample_features", EValueType::VT_ANY);
            schema.SortBy({});
            preparer.OutputSchema(/* tableIndex */ 0, schema);
        }

    private:
        ELanguage Language;

        TNode SampleInfo(const TNode& node) const {
            TNode sampleInfo;

            const TNode::TListType& tokenNodes = node["embeddings"].AsList();
            TVector<TString> tokens(Reserve(tokenNodes.size()));

            for (const TNode& tokenNode : tokenNodes) {
                tokens.emplace_back(tokenNode["token"].AsString());
            }

            const TVector<TString> tokenTags = NVins::GetTags(node["markup"].AsString(), tokens, Language);
            Y_ENSURE(tokenTags.size() == tokens.size());

            sampleInfo["tokens"] = TNode::CreateList();
            sampleInfo["tags"] = TNode::CreateList();

            for (const auto& [tokenNode, tokenTag] : Zip(tokenNodes, tokenTags)) {
                const TNode::TListType& subtokenNodes = tokenNode["subtokens"].AsList();

                for (size_t subtokenIdx = 0; subtokenIdx < subtokenNodes.size(); ++subtokenIdx) {
                    sampleInfo["tokens"].Add(subtokenNodes[subtokenIdx]["subtoken"]);

                    if (tokenTag.StartsWith("B") && subtokenIdx != 0) {
                        sampleInfo["tags"].Add("I" + tokenTag.substr(1));
                    } else {
                        sampleInfo["tags"].Add(tokenTag);
                    }
                }
            }

            sampleInfo["utterance"]["text"] = node["utterance_text"];

            return sampleInfo;
        }

        TNode DenseSeqFeatures(const TNode& node) const {
            size_t subtokensCount = 0;

            TNode embFeatures;
            embFeatures["key"] = "alice_requests_emb";
            embFeatures["value"]["data"] = TNode::CreateList();

            for (const TNode& tokenNode : node["embeddings"].AsList()) {
                subtokensCount += tokenNode["subtokens"].AsList().size();

                for (const TNode& subtokenNode : tokenNode["subtokens"].AsList()) {
                    for (const TNode& val : subtokenNode["embedding"].AsList()) {
                        embFeatures["value"]["data"].Add(val);
                    }
                }
            }

            const size_t dataSize = embFeatures["value"]["data"].AsList().size();
            Y_ENSURE(dataSize % subtokensCount == 0);

            embFeatures["value"]["shape_x"] = subtokensCount;
            embFeatures["value"]["shape_y"] = dataSize / subtokensCount;

            return TNode::CreateList({embFeatures});
        }

        TNode SparseSeqFeaturesStub(const size_t subtokensCount) const {
            TNode sparseSeqFeatures = TNode::CreateList();

            const TNode stub = TNode()("data", TNode::CreateList(TNode::TListType(subtokensCount, TNode::CreateMap())));

            for (const TStringBuf key : {"ner", "postag", "wizard"}) {
                TNode featureNode;
                featureNode["key"] = key;
                featureNode["value"] = stub;

                sparseSeqFeatures.Add(std::move(featureNode));
            }

            return sparseSeqFeatures;
        }
    };

    REGISTER_MAPPER(TConverter);
}

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

    TString langName;
    options.AddLongOption('l', "lang", "language")
        .Required()
        .RequiredArgument("LANGUAGE_NAME")
        .StoreResult(&langName);

    NLastGetopt::TOptsParseResult parseOpts{&options, argc, argv};

    const ELanguage lang = LanguageByNameOrDie(langName);
    const auto client = NYT::CreateClient(proxyName);

    client->Map(
        NYT::TMapOperationSpec()
            .AddInput<TNode>(inputTable)
            .AddOutput<TNode>(outputTable)
            .Ordered(true),
        MakeIntrusive<TConverter>(lang)
    );

    return 0;
}
