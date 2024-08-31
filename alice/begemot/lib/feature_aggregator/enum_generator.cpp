#include "enum_generator.h"

#include "config.h"

#include <library/cpp/iterator/mapped.h>

#include <util/generic/vector.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/join.h>

#include <utility>

namespace NAlice::NFeatureAggregator {

namespace {

using TFeature = TFeatureAggregatorConfig::TFeature;
using TNameIndex = TVector<std::pair<TStringBuf, uint>>;

inline constexpr TStringBuf INDENT("    ");
inline constexpr int BASE_10 = 10;


TString GenerateReservedValues(const TNameIndex& disabledFeatures) {
    if (disabledFeatures.empty()) {
        return {};
    }

    TVector<TString> indexes;
    TVector<TString> names;
    for (const auto& [name, index] : disabledFeatures) {
        indexes.push_back(IntToString<BASE_10>(index));
        names.push_back(TString::Join('"', name, '"'));
    }

    return TString::Join(
        INDENT, "reserved ", JoinSeq(", ", indexes), ";\n",
        INDENT, "reserved ", JoinSeq(", ", names), ";\n"
    );
}

TString GenerateEnumEntry(const std::pair<TStringBuf, uint>& nameIndex) {
    return TString::Join(INDENT, nameIndex.first, " = ", IntToString<BASE_10>(nameIndex.second), ";");
}

TString GenerateActualValues(const TNameIndex& enabledFeatures) {
    Y_ENSURE(!enabledFeatures.empty());
    return JoinSeq("\n", MakeMappedRange(enabledFeatures, GenerateEnumEntry)) + "\n";
}

} // namespace

TString GenerateProtoEnum(
    const TFeatureAggregatorConfig& config,
    const TStringBuf enumName,
    const TMaybe<TStringBuf> protoPackage,
    const TMaybe<TStringBuf> goPackage,
    const TMaybe<TStringBuf> javaPackage
) {
    TNameIndex enabledFeatures = {{RESERVED_FEATURE_NAME, 0}};
    TNameIndex disabledFeatures;

    for (const TFeature& feature : config.GetFeatures()) {
        if (feature.GetIsDisabled()) {
            disabledFeatures.emplace_back(feature.GetName(), feature.GetIndex());
        } else {
            enabledFeatures.emplace_back(feature.GetName(), feature.GetIndex());
        }
    }

    TStringBuilder builder;
    builder << "syntax = \"proto3\";\n";

    if (protoPackage.Defined()) {
        builder << "package " << protoPackage.GetRef() << ";\n";
    }

    if (goPackage.Defined()) {
        builder << "option go_package = \"" << goPackage.GetRef() << "\"" << ";\n";
    }

    if (javaPackage.Defined()) {
        builder << "option java_package = \"" << javaPackage.GetRef() << "\"" << ";\n";
    }

    builder << "enum " << enumName << " {\n";

    builder << GenerateReservedValues(disabledFeatures);
    builder << GenerateActualValues(enabledFeatures);

    builder << "}\n";

    return builder;
}

} // namespace NAlice::NFeatureAggregator
