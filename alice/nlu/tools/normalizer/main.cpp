#include <alice/nlu/libs/request_normalizer/request_normalizer.h>
#include <alice/nlu/libs/request_normalizer/request_tokenizer.h>
#include <mapreduce/yt/interface/logging/logger.h>
#include <mapreduce/yt/util/temp_table.h>
#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/langs/langs.h>
#include <util/generic/serialized_enum.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/split.h>

struct TNormalizerYTMapperOptions {
    TString InputColumn;
    TString OutputColumn;
    bool UseNluTags;
    ELanguage Language;

    Y_SAVELOAD_DEFINE(InputColumn,
                      OutputColumn,
                      UseNluTags,
                      Language);
};

static bool HasNluTags(const NYT::TNode& inRow) {
    return inRow.HasKey("additional_options") &&
        inRow["additional_options"].IsMap() &&
        inRow["additional_options"].HasKey("nlu_tags") &&
        inRow["additional_options"]["nlu_tags"].IsList();
}

static TString GetContinuingTag(const TString& slotValue) {
    if (slotValue == "O") {
        return slotValue;
    }
    return "I-" + slotValue;
}

static TString GetSlotValue(const TString& tag) {
    if (tag == "O") {
        return tag;
    }
    Y_VERIFY(tag.size() >= 2);
    return tag.substr(2, tag.size() - 2); // "I-" or "B-"
}

class TNormalizerYTMapper : public NYT::IMapper<NYT::TNodeReader, NYT::TNodeWriter> {
public:
    TNormalizerYTMapper()
    {
    }

    TNormalizerYTMapper(const TNormalizerYTMapperOptions& options)
        : Options(options)
    {
    }

    void PrepareOperation(const NYT::IOperationPreparationContext& context, NYT::TJobOperationPreparer& preparer) const override {
        NYT::TTableSchema tableSchema = context.GetInputSchema(/* tableIndex */ 0);
        tableSchema.AddColumn(Options.OutputColumn, NYT::EValueType::VT_STRING);
        preparer.OutputSchema(/* tableIndex */ 0, tableSchema);
    }

    void Do(TReader* input, TWriter* output) override {
        for (; input->IsValid(); input->Next()) {
            const auto& inRow = input->GetRow();
            TString request = inRow[Options.InputColumn].AsString();

            NYT::TNode outRow(inRow);
            TString normalizedRequest;

            if (Options.UseNluTags && HasNluTags(inRow)) {
                const TVector<TString> tokens = StringSplitter(request).Split(' ').SkipEmpty();

                TVector<TString> tags;
                for (const auto& tag : inRow["additional_options"]["nlu_tags"].AsList()) {
                    tags.push_back(tag.AsString());
                }

                if (tags.size() != tokens.size()) {
                    continue;
                }

                TVector<TString> normalizedTags;
                NormalizeTaggedRequest(tokens, tags, &normalizedRequest, &normalizedTags);
                auto& outTags = outRow["additional_options"]["nlu_tags"].AsList();
                outTags.clear();
                for (const auto& tag : normalizedTags) {
                    outTags.push_back(tag);
                }

                const size_t normalizedTokenCount = StringSplitter(normalizedRequest).Split(' ').SkipEmpty().Count();
                const size_t normalizedTagCount = outRow["additional_options"]["nlu_tags"].Size();
                Y_VERIFY(normalizedTokenCount == normalizedTagCount);
            } else {
                normalizedRequest = NNlu::TRequestNormalizer::Normalize(Options.Language, request);
            }

            outRow[Options.OutputColumn] = normalizedRequest;

            output->AddRow(outRow);
        }
    }

    Y_SAVELOAD_JOB(Options);

private:
    void NormalizeTaggedRequest(const TVector<TString>& tokens,
                                const TVector<TString>& tags,
                                TString* normalizedRequest,
                                TVector<TString>* normalizedTags) const {
        if (tags.size() == 0) {
            *normalizedRequest = "";
            *normalizedTags = {};
            return;
        }

        TVector<TString> normalizedSlots;
        size_t slotStart = 0;
        TString slotValue = GetSlotValue(tags[0]);
        for (size_t slotEnd = 1; slotEnd <= tags.size(); ++slotEnd) {
            TString nextTagValue = (slotEnd == tags.size()) ? "" : GetSlotValue(tags[slotEnd]);

            if (nextTagValue == slotValue) {
                continue; // still growing the slot
            }

            const auto slotText = JoinRange(" ", tokens.begin() + slotStart, tokens.begin() + slotEnd);
            const auto normalizedSlotText = NNlu::TRequestNormalizer::Normalize(Options.Language, slotText);
            normalizedSlots.push_back(normalizedSlotText);
            const size_t numSlotTokens = StringSplitter(normalizedSlotText).Split(' ').SkipEmpty().Count();

            normalizedTags->push_back(tags[slotStart]);
            normalizedTags->resize(normalizedTags->size() + numSlotTokens - 1, GetContinuingTag(slotValue));

            slotStart = slotEnd;
            slotValue = nextTagValue;
        }

        *normalizedRequest = JoinSeq(" ", normalizedSlots);
    }

private:
    TNormalizerYTMapperOptions Options;
};
REGISTER_MAPPER(TNormalizerYTMapper);

struct TNormalizerToolOptions {
    TString Proxy;
    TString InputPath;
    TString OutputPath;
    NYT::ILogger::ELevel YTLogLevel;
    TNormalizerYTMapperOptions MapperOptions;
};

TNormalizerToolOptions ReadNormalizerToolOptions(int argc, const char** argv) {
    TNormalizerToolOptions result;
    TString language;
    auto parser = NLastGetopt::TOpts();

    parser.AddLongOption("proxy")
          .StoreResult(&result.Proxy)
          .Required()
          .Help("YT proxy.");

    parser.AddLongOption("input")
          .AddShortName('i')
          .StoreResult(&result.InputPath)
          .Required()
          .Help("Input table absolute path.");

    parser.AddLongOption("output")
          .AddShortName('o')
          .StoreResult(&result.OutputPath)
          .Required()
          .Help("Output table absolute path.");

    parser.AddLongOption("input-column")
          .StoreResult(&result.MapperOptions.InputColumn)
          .DefaultValue("text")
          .Help("Input table column name.");

    parser.AddLongOption("output-column")
          .StoreResult(&result.MapperOptions.OutputColumn)
          .DefaultValue("text")
          .Help("Output table column name.");

    parser.AddLongOption("use-nlu-tags")
          .StoreTrue(&result.MapperOptions.UseNluTags)
          .DefaultValue(false)
          .Help("Add this argument to normalize each slot separately."
                "Needs 'additional_options' column with 'nlu_tags' key and list of token tags as a value.");

    parser.AddLongOption("language")
          .StoreResult(&language)
          .DefaultValue("ru")
          .Help("Language code.");

    parser.AddLongOption("yt-log-level")
          .RequiredArgument("ENUM")
          .DefaultValue(ToString(NYT::ILogger::INFO))
          .StoreResult(&result.YTLogLevel)
          .Help("YT log level, one of: " + GetEnumAllNames<NYT::ILogger::ELevel>());

    parser.AddHelpOption();
    parser.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult parserResult{&parser, argc, argv};
    result.MapperOptions.Language = LanguageByName(language);

    return result;
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    const auto options = ReadNormalizerToolOptions(argc, argv);

    NYT::SetLogger(NYT::CreateStdErrLogger(options.YTLogLevel));
    const auto client = NYT::CreateClient(options.Proxy);

    NYT::TMapOperationSpec spec;
    spec.AddInput<NYT::TNode>(options.InputPath)
        .AddOutput<NYT::TNode>(options.OutputPath)
        .Ordered(true);
        // TODO(smirnovpavel): estimate memory usage

    client->Map(spec, new TNormalizerYTMapper(options.MapperOptions));

    return 0;
}
