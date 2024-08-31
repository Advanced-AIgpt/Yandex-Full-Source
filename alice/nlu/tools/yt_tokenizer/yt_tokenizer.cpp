#include <alice/nlu/libs/tokenization/tokenizer.h>

#include <mapreduce/yt/interface/client.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/string/cast.h>

template <>
ELanguage FromString<ELanguage>(const TStringBuf& value) {
    return LanguageByNameOrDie(value);
}

namespace {
    struct TTokenizerMapperOptions {
        TString InputColumn;
        TString OutputColumn;
        NNlu::ETokenizerType TokenizerType = NNlu::ETokenizerType::DEFAULT;
        ELanguage Language = ELanguage::LANG_RUS;
        bool NormalizeTokens = false;

        Y_SAVELOAD_DEFINE(
            InputColumn,
            OutputColumn,
            TokenizerType,
            Language,
            NormalizeTokens
        );
    };

    struct TTokenizerToolOptions {
        TTokenizerMapperOptions MapperOptions;
        TString Proxy;
        TString InputPath;
        TString OutputPath;

        TTokenizerToolOptions(int argc, const char** argv) {
            NLastGetopt::TOpts opts;

            opts.SetFreeArgsNum(0);

            opts.AddLongOption('p', "proxy", "YT proxy name")
                .StoreResult(&Proxy)
                .Required()
                .RequiredArgument("PROXY");

            opts.AddLongOption('i', "input", "input table")
                .StoreResult(&InputPath)
                .Required()
                .RequiredArgument("YPATH");

            opts.AddLongOption('o', "output", "output table")
                .StoreResult(&OutputPath)
                .Required()
                .RequiredArgument("YPATH");

            opts.AddLongOption("input-column", "column name with input text to tokenize")
                .StoreResult(&MapperOptions.InputColumn)
                .Required()
                .RequiredArgument("COLUMN");

            opts.AddLongOption("output-column", "output column name with resulting tokens")
                .StoreResult(&MapperOptions.OutputColumn)
                .Required()
                .RequiredArgument("COLUMN");

            opts.AddLongOption("tokenizer-type", "tokenizer type: either 'smart' or 'whitespace'")
                .StoreResult(&MapperOptions.TokenizerType)
                .Required()
                .RequiredArgument("TOKENIZER_TYPE");

            opts.AddLongOption("language", "language code")
                .StoreResult(&MapperOptions.Language)
                .Required()
                .RequiredArgument("LANGUAGE");

            opts.AddLongOption("normalize", "normalize tokens after tokenization")
                .StoreTrue(&MapperOptions.NormalizeTokens);

            NLastGetopt::TOptsParseResult parseOpts{&opts, argc, argv};
        }
    };

    class TTokenizerMapper: public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
    public:
        TTokenizerMapper() = default;

        explicit TTokenizerMapper(const TTokenizerMapperOptions& options)
            : Options(options)
        {
        }

        Y_SAVELOAD_JOB(Options);

        void Do(TReader* reader, TWriter* writer) override {
            for (const auto& cursor : *reader) {
                NYT::TNode row = cursor.GetRow();

                auto tokenizer = NNlu::CreateTokenizer(
                    Options.TokenizerType, row[Options.InputColumn].AsString(), Options.Language);

                TVector<TString> tokens;

                if (Options.NormalizeTokens) {
                    tokens = tokenizer->GetNormalizedTokens();
                } else {
                    tokens = tokenizer->GetOriginalTokens();
                }

                row[Options.OutputColumn] = NYT::TNode::CreateList();

                for (const TString& token : tokens) {
                    row[Options.OutputColumn].Add(token);
                }

                writer->AddRow(row);
            }
        }

        void PrepareOperation(const NYT::IOperationPreparationContext& context, NYT::TJobOperationPreparer& preparer) const override {
            NYT::TTableSchema tableSchema = context.GetInputSchema(/* tableIndex */ 0);
            tableSchema.AddColumn(Options.OutputColumn, NYT::EValueType::VT_ANY);
            preparer.OutputSchema(/* tableIndex */ 0, tableSchema);
        }

    private:
        TTokenizerMapperOptions Options;
    };

    REGISTER_MAPPER(TTokenizerMapper);
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);
    const TTokenizerToolOptions options(argc, argv);

    const NYT::IClientPtr client = NYT::CreateClient(options.Proxy);

    client->Map(
        NYT::TMapOperationSpec()
            .AddInput<NYT::TNode>(options.InputPath)
            .AddOutput<NYT::TNode>(options.OutputPath)
            .Ordered(true),
        MakeIntrusive<TTokenizerMapper>(options.MapperOptions)
    );

    return 0;
}
