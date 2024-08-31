#include "config.h"

#include <alice/begemot/lib/fresh_options/fresh_options.h>

namespace NAlice {

namespace {

void ValidateSource(const TStringBuf source) {
    Y_ENSURE_EX(!source.empty(), TFrameConfigValidationException() << "Source is not defined");

    const EFrameAggregatorSourceName sourceName = ReadFrameAggregatorSourceName(source);
    Y_ENSURE_EX(sourceName != EFrameAggregatorSourceName::UNKNOWN,
        TFrameConfigValidationException() << "Unknown source: " << source);
}

void ValidateClassifierRule(const TFrameAggregatorConfig::TClassifierRule& rule) {
    ValidateSource(rule.GetSource());

    const EFrameAggregatorSourceName sourceName = ReadFrameAggregatorSourceName(rule.GetSource());

    if (sourceName == EFrameAggregatorSourceName::ALICE_MULTI_INTENT_CLASSIFIER ||
        sourceName == EFrameAggregatorSourceName::ALICE_WIZ_DETECTION)
    {
        Y_ENSURE_EX(!rule.GetModel().empty(),
            TFrameConfigValidationException() << "Empty model for source: " << rule.GetSource());
    }

    if (sourceName == EFrameAggregatorSourceName::ALICE_WIZ_DETECTION) {
        Y_ENSURE_EX(rule.GetIntent().empty(),
            TFrameConfigValidationException() << "Non-empty intent for source: " << rule.GetSource());
    }
}


void ValidateTaggerRule(const TFrameAggregatorConfig::TTaggerRule& rule) {
    ValidateSource(rule.GetSource());
}

void ValidateFrameConfig(const TFrameAggregatorConfig::TFrame& frameConfig) {
    // TODO(samoylovboris) Remove rule cascade from config of alice.market.how_much.
    const bool isWeakValidation = frameConfig.GetName() == "alice.market.how_much" ||
        frameConfig.GetName() == "alice.apps_fixlist";

    Y_ENSURE_EX(!frameConfig.GetName().empty(), TFrameConfigValidationException() << "Frame name is not defined");
    Y_ENSURE_EX(isWeakValidation || frameConfig.GetRules().size() <= 1, TFrameConfigValidationException() <<
        "Multiple rules is a hidden feature. Let me know if you need it (samoylovboris)");

    for (const TFrameAggregatorConfig::TRule& ruleConfig : frameConfig.GetRules()) {
        Y_ENSURE_EX(isWeakValidation || !ruleConfig.GetClassifier().GetIsNegative(), TFrameConfigValidationException() <<
            "Negative rule is a hidden feature. Let me know if you need it (samoylovboris)");

        ValidateClassifierRule(ruleConfig.GetClassifier());

        if (ruleConfig.HasTagger()){
            ValidateTaggerRule(ruleConfig.GetTagger());
        }
    }
}

void AddLoggingThresholdDefaultValue(TFrameAggregatorConfig& config) {
    for (TFrameAggregatorConfig::TFrame& frameConfig : *config.MutableFrames()) {
        for (TFrameAggregatorConfig::TRule& ruleConfig : *frameConfig.MutableRules()) {
            TFrameAggregatorConfig::TClassifierRule& classifierRule = *ruleConfig.MutableClassifier();
            if (!classifierRule.HasLoggingThreshold()) {
                classifierRule.SetLoggingThreshold(classifierRule.GetThreshold());
            }
        }
    }
}

} // namespace

TFrameAggregatorConfig ReadFrameAggregatorConfigFromProtoTxtString(TStringBuf str) {
    auto result = ParseProtoText<TFrameAggregatorConfig>(str);
    AddLoggingThresholdDefaultValue(result);
    return result;
}

THashMap<ELanguage, TFrameAggregatorConfig> LoadFrameAggregatorConfigs(const NBg::TFileSystem& fs, const TFsPath& dir) {
    if (!fs.Exists(dir)) {
        return {};
    }
    auto configs = LoadLanguageConfigs<TFrameAggregatorConfig>(*fs.Subdirectory(dir), "frames.pb.txt");
    for (auto& [_, config] : configs) {
        AddLoggingThresholdDefaultValue(config);
    }
    return configs;
}

void Validate(const TFrameAggregatorConfig& config) {
    for (const TFrameAggregatorConfig::TFrame& frameConfig : config.GetFrames()) {
        ValidateFrameConfig(frameConfig);
    }
}

TFrameAggregatorConfig MergeFrameAggregatorConfigs(
    const TFrameAggregatorConfig* staticConfig,
    const TFrameAggregatorConfig* freshConfig,
    const TFrameAggregatorConfig* devConfig,
    const TVector<TFrameAggregatorConfig>& configPatches,
    const TFreshOptions& freshOptions)
{
    TFrameAggregatorConfig resultConfig;
    for (const TFrameAggregatorConfig& configPatch : configPatches) {
        if (resultConfig.GetLanguage().empty()) {
            resultConfig.SetLanguage(configPatch.GetLanguage());
        }
        for (const TFrameAggregatorConfig::TFrame& frameConfig : configPatch.GetFrames()) {
            *resultConfig.AddFrames() = frameConfig;
        }
    }
    if (devConfig != nullptr) {
        if (resultConfig.GetLanguage().empty()) {
            resultConfig.SetLanguage(devConfig->GetLanguage());
        }
        for (const TFrameAggregatorConfig::TFrame& frameConfig : devConfig->GetFrames()) {
            *resultConfig.AddFrames() = frameConfig;
        }
    }
    if (freshConfig != nullptr) {
        if (resultConfig.GetLanguage().empty()) {
            resultConfig.SetLanguage(freshConfig->GetLanguage());
        }
        for (const TFrameAggregatorConfig::TFrame& frameConfig : freshConfig->GetFrames()) {
            if (ShouldUseFreshForForm(freshOptions, frameConfig.GetName())) {
                *resultConfig.AddFrames() = frameConfig;
            }
        }
    }
    if (staticConfig != nullptr) {
        if (resultConfig.GetLanguage().empty()) {
            resultConfig.SetLanguage(staticConfig->GetLanguage());
        }
        for (const TFrameAggregatorConfig::TFrame& frameConfig : staticConfig->GetFrames()) {
            if (freshConfig == nullptr || !ShouldUseFreshForForm(freshOptions, frameConfig.GetName())) {
                *resultConfig.AddFrames() = frameConfig;
            }
        }
    }
    return resultConfig;
}

} // namespace NAlice
