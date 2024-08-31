#include <alice/begemot/lib/feature_aggregator/config.h>
#include <alice/begemot/lib/feature_aggregator/enum_generator.h>

#include <library/cpp/getopt/small/last_getopt.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/stream/input.h>
#include <util/system/file.h>

namespace {

struct TOptions {
    TString ConfigPath;
    TString OutputProtoPath;
    TString EnumName;
    TMaybe<TString> ProtoPackage = Nothing();
    TMaybe<TString> GoPackage = Nothing();
    TMaybe<TString> JavaPackage = Nothing();
};

TOptions ReadOptions(int argc, const char** argv) {
    TOptions result;
    NLastGetopt::TOpts parser;

    parser.SetTitle("Generate protobuf with enum to access features from FeatureAggregator output");

    parser.SetFreeArgsNum(3);

    parser.GetFreeArgSpec(0)
        .Title("input-config-path")
        .Help("Path to input FeatureAggregator config");

    parser.GetFreeArgSpec(1)
        .Title("output-proto-path")
        .Help("Path to output .proto file");

    parser.GetFreeArgSpec(2)
        .Title("enum-name")
        .Help("Name of generated enum");

    parser.AddLongOption("proto-package", "Proto package in generated enum");
    parser.AddLongOption("go-package", "Go package in generated enum");
    parser.AddLongOption("java-package", "Java package in generated enum");

    parser.AddHelpOption('h');
    NLastGetopt::TOptsParseResult parserResult{&parser, argc, argv};

    result.ConfigPath = parserResult.GetFreeArgs()[0];
    result.OutputProtoPath = parserResult.GetFreeArgs()[1];
    result.EnumName = parserResult.GetFreeArgs()[2];

    if (parserResult.Has("proto-package")) {
        result.ProtoPackage = parserResult.Get("proto-package");
    }

    if (parserResult.Has("go-package")) {
        result.GoPackage = parserResult.Get("go-package");
    }

    if (parserResult.Has("java-package")) {
        result.JavaPackage = parserResult.Get("java-package");
    }

    return result;
}

} // namespace

int main(int argc, const char** argv) {
    const TOptions options = ReadOptions(argc, argv);

    const TFile configFile(options.ConfigPath, OpenExisting | RdOnly);
    const TFile outputProtoFile(options.OutputProtoPath, CreateNew | WrOnly);

    const TString rawConfig = TFileInput(configFile).ReadAll();
    const NAlice::TFeatureAggregatorConfig cfg = NAlice::NFeatureAggregator::ReadConfigFromProtoTxtString(rawConfig);

    TFileOutput protoFileOutput(outputProtoFile);

    protoFileOutput << NAlice::NFeatureAggregator::GenerateProtoEnum(
        cfg, options.EnumName, options.ProtoPackage, options.GoPackage, options.JavaPackage);
}
