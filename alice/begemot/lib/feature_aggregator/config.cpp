#include "config.h"

#include <alice/begemot/lib/utils/config.h>
#include <alice/library/proto/proto.h>

#include <library/cpp/iterator/enumerate.h>

#include <util/digest/sequence.h>
#include <util/generic/algorithm.h>
#include <util/generic/cast.h>
#include <util/generic/hash_set.h>
#include <util/string/join.h>

#include <limits>
#include <regex>

namespace NAlice::NFeatureAggregator {

namespace {

using TFeature = TFeatureAggregatorConfig::TFeature;
using TRule = TFeatureAggregatorConfig::TRule;
using TIntervalMapping = TFeatureAggregatorConfig::TIntervalMapping;

constexpr TStringBuf VALID_NAME_RAW_RE = "[0-9a-z_-]+";
const std::regex VALID_NAME_RE(VALID_NAME_RAW_RE.Data(), std::regex::icase);

void CheckValidNames(const TFeatureAggregatorConfig& cfg) {
    for (const TFeature& feature : cfg.GetFeatures()) {
        Y_ENSURE_EX(std::regex_match(feature.GetName().data(), VALID_NAME_RE),
            TValidationException() << "feature " << feature.GetName() << " name doesn't match " << VALID_NAME_RAW_RE);

        Y_ENSURE_EX(feature.GetName() != RESERVED_FEATURE_NAME,
            TValidationException() << RESERVED_FEATURE_NAME << " feature name is not allowed, reserved for enum");
    }
}

void CheckValidIndexes(const TFeatureAggregatorConfig& cfg) {
    for (const TFeature& feature : cfg.GetFeatures()) {
        Y_ENSURE_EX(feature.GetIndex() > 0,
            TValidationException() << "feature " << feature.GetName() << " has non-positive index");
        Y_ENSURE_EX(feature.GetIndex() < std::numeric_limits<google::protobuf::int32>::max(),
            TValidationException() << "feature " << feature.GetName() << " index leads to vector size overflow");
    }
}

void CheckSameIndexes(const TFeatureAggregatorConfig& cfg) {
    THashSet<google::protobuf::uint32> indexSet;

    for (const TFeature& feature : cfg.GetFeatures()) {
        auto [_, inserted] = indexSet.insert(feature.GetIndex());

        Y_ENSURE_EX(inserted, TValidationException() << "feature " << feature.GetName() << " contains existing index");
    }
}

void CheckSortedIndexes(const TFeatureAggregatorConfig& cfg) {
    TVector<google::protobuf::uint32> indexes;

    for (const TFeature& feature : cfg.GetFeatures()) {
        indexes.push_back(feature.GetIndex());
    }

    Y_ENSURE_EX(IsSorted(indexes.begin(), indexes.end()), TValidationException() << "feature indexes are not sorted");
}

void CheckSameNames(const TFeatureAggregatorConfig& cfg) {
    THashSet<TString> nameSet;

    for (const TFeature& feature : cfg.GetFeatures()) {
        auto [_, inserted] = nameSet.insert(feature.GetName());

        Y_ENSURE_EX(inserted, TValidationException() << "same feature names in config: " << feature.GetName());
    }
}

void CheckEmptyRules(const TFeatureAggregatorConfig& cfg) {
    for (const TFeature& feature : cfg.GetFeatures()) {
        Y_ENSURE_EX(feature.GetIsDisabled() || !feature.GetRules().empty(),
            TValidationException() << "feature is not disabled and contains empty rules: " << feature.GetName());
    }
}

void CheckMainRuleAndExperimentsForFeature(const TFeature& feature) {
    if (feature.GetIsDisabled()) {
        return;
    }

    THashSet<TVector<TStringBuf>, TRangeHash<THash<TStringBuf>>> experimentSet;

    for (const TRule& rule : feature.GetRules()) {
        TVector<TStringBuf> ruleExperiments(rule.GetExperiments().begin(), rule.GetExperiments().end());
        SortUnique(ruleExperiments);

        if (auto [it, inserted] = experimentSet.insert(ruleExperiments); !inserted) {
            const auto experimentsStr =
                Accumulate(ruleExperiments, TStringBuf(),
                           [](const TStringBuf& cur, const TStringBuf& next) { return Join(cur, " ", next); });

            ythrow TValidationException()
                << "feature " << feature.GetName() << " contains two rules for same experiment: " << experimentsStr;
        }
    }
}

void CheckMainRuleAndExperiments(const TFeatureAggregatorConfig& cfg) {
    for (const TFeature& feature : cfg.GetFeatures()) {
        CheckMainRuleAndExperimentsForFeature(feature);
    }
}

void CheckIntervalMapping(const TStringBuf featureName, const TIntervalMapping& mapping) {
    Y_ENSURE_EX(!mapping.GetPoints().empty(), TValidationException() <<  featureName << ": has intervalMapping but Points array is empty");

    TVector<float> thresholds;
    thresholds.reserve(mapping.PointsSize());

    for (const TIntervalMapping::TPoint& point : mapping.GetPoints()) {
        thresholds.push_back(point.GetThreshold());
    }

    TVector<float> sortedUniqueThresholds = thresholds;
    SortUnique(sortedUniqueThresholds);

    Y_ENSURE_EX(Equal(thresholds.begin(), thresholds.end(), sortedUniqueThresholds.begin(), sortedUniqueThresholds.end()),
        TValidationException() << featureName << ": contains either same or unsorted thresholds");
}

template <typename TRuleWithFeatureValueOption>
void CheckFeatureValueOptionForRule(const TStringBuf featureName, const TRuleWithFeatureValueOption& rule) {
    switch (rule.GetFeatureValueCase()) {
        case TRuleWithFeatureValueOption::FeatureValueCase::FEATUREVALUE_NOT_SET: {
            ythrow TValidationException() << featureName << ": FeatureValue is not set";
        }
        case TRuleWithFeatureValueOption::FeatureValueCase::kUseRawValue: {
            Y_ENSURE_EX(rule.GetUseRawValue(), TValidationException() << featureName << ": UseRawValue is chosen but flag is false");
            break;
        }
        case TRuleWithFeatureValueOption::FeatureValueCase::kIntervalMapping: {
            CheckIntervalMapping(featureName, rule.GetIntervalMapping());
        }
    }
}

void CheckFeatureValueOption(const TFeatureAggregatorConfig& cfg) {
    for (const TFeature& feature : cfg.GetFeatures()) {
        for (const TRule& rule : feature.GetRules()) {
            switch (rule.GetSourceCase()) {
                case TRule::SourceCase::SOURCE_NOT_SET:
                case TRule::SourceCase::kEntity:
                case TRule::SourceCase::kGranetFrame:
                case TRule::SourceCase::kPornQuery:
                case TRule::SourceCase::kWizDetection: {
                    break;
                }
                case TRule::SourceCase::kMultiIntentClassifier: {
                    CheckFeatureValueOptionForRule(feature.GetName(), rule.GetMultiIntentClassifier());
                    break;
                }
                case TRule::SourceCase::kBinaryClassifier: {
                    CheckFeatureValueOptionForRule(feature.GetName(), rule.GetBinaryClassifier());
                    break;
                }
            }
        }
    }
}

bool AreExperimentsSubsetOf(const TRule& rule, const TRule& otherRule) {
    const THashSet<TString> ruleExperiments(rule.GetExperiments().begin(), rule.GetExperiments().end());
    const THashSet<TString> otherRuleExperiments(otherRule.GetExperiments().begin(), otherRule.GetExperiments().end());

    return AllOf(ruleExperiments,
                 [&otherRuleExperiments](const auto& exp) { return otherRuleExperiments.contains(exp); });
}

void CheckPrioritiesForFeature(const TFeature& feature) {
    const auto& rules = feature.GetRules();

    for (auto [i, rule] : Enumerate(rules)) {
        for (auto j = SafeIntegerCast<int>(i) + 1; j < rules.size(); ++j) {
            const auto& lowerPriorityRule = rules[j];

            Y_ENSURE_EX(!AreExperimentsSubsetOf(rule, lowerPriorityRule),
                TValidationException() << "lower priority rule " << lowerPriorityRule <<
                " won't work as it includes experiments from rule " << rule);
        }
    }
}

void CheckPriorities(const TFeatureAggregatorConfig& cfg) {
    for (const TFeature& feature : cfg.GetFeatures()) {
        CheckPrioritiesForFeature(feature);
    }
}

void CheckUnknownRulesForFeature(const TFeature& feature) {
    for (const auto& rule : feature.GetRules()) {
        Y_ENSURE_EX(rule.GetSourceCase() != TRule::SOURCE_NOT_SET, TValidationException() << "unknown rule in config: " << rule);
    }
}

void CheckUnknownRules(const TFeatureAggregatorConfig& cfg) {
    for (const TFeature& feature : cfg.GetFeatures()) {
        CheckUnknownRulesForFeature(feature);
    }
}

void CheckExperimentsForFeature(const TFeature& feature) {
    for (const auto& rule : feature.GetRules()) {
        for (const auto& experiment : rule.GetExperiments()) {
            Y_ENSURE_EX(experiment.find('=') == TString::npos, TValidationException() << "Experiments shouldn't contain '='. Error in rule:" << rule);
        }
    }
}

void CheckExperiments(const TFeatureAggregatorConfig& cfg) {
    for (const TFeature& feature : cfg.GetFeatures()) {
        CheckExperimentsForFeature(feature);
    }
}

void Validate(const TFeatureAggregatorConfig& cfg, bool ignoreUnknownRules) {
    CheckValidNames(cfg);
    CheckValidIndexes(cfg);
    CheckSameIndexes(cfg);
    CheckSortedIndexes(cfg);
    CheckSameNames(cfg);
    CheckEmptyRules(cfg);
    CheckMainRuleAndExperiments(cfg);
    CheckFeatureValueOption(cfg);
    CheckPriorities(cfg);
    CheckExperiments(cfg);
    if (!ignoreUnknownRules) {
        CheckUnknownRules(cfg);
    }

}

} // namespace

TFeatureAggregatorConfig ReadConfigFromProtoTxtString(const TStringBuf str, bool ignoreUnknownRules) {
    auto cfg = ParseProtoText<TFeatureAggregatorConfig>(str,  /* permissive */ ignoreUnknownRules);
    Validate(cfg, ignoreUnknownRules);
    return cfg;
}

THashMap<ELanguage, TFeatureAggregatorConfig> LoadConfigs(const NBg::TFileSystem& fs, bool loadFreshConfig) {
    const TFsPath& dir = loadFreshConfig ? "fresh" : "static";

    if (!fs.Exists(dir)) {
        return {};
    }

    auto configMap = LoadLanguageConfigs<TFeatureAggregatorConfig>(*fs.Subdirectory(dir), "features.pb.txt", /* permissive */ loadFreshConfig);

    for (const auto& [language, config] : configMap) {
        Validate(config, /* ignoreUnknownRules */ loadFreshConfig);
    }
    return configMap;
}

} // namespace NAlice::NFeatureAggregator
