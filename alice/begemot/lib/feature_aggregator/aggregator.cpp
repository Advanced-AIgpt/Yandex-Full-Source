#include "aggregator.h"

#include <library/cpp/iterator/enumerate.h>
#include <library/cpp/iterator/zip.h>

#include <util/generic/algorithm.h>
#include <util/generic/mapfindptr.h>
#include <util/generic/maybe.h>

namespace NAlice::NFeatureAggregator {

namespace {

inline constexpr float DEFAULT_FEATURE_VALUE = 0.0F;
/* TODO(vl-trifonov)
May be we should distuingish Granet frames in interface or make all frames possible to extract features
*/
inline constexpr TStringBuf GRANET_SOURCE_LABEL = "Granet";

using TFeature = TFeatureAggregatorConfig::TFeature;
using TRule = TFeatureAggregatorConfig::TRule;
using TIntervalMapping = TFeatureAggregatorConfig::TIntervalMapping;
using TBinaryClassifierRule = TRule::TBinaryClassifierRule;
using TProtoRepeatedString = google::protobuf::RepeatedPtrField<TProtoStringType>;
using TProtoRepeatedFeature = google::protobuf::RepeatedPtrField<TFeature>;
using TProtoRepeatedFloat = google::protobuf::RepeatedField<float>;

inline float BoolToValue(bool b) {
    return b ? 1.0F : 0.0F;
}

template <typename TRuleWithIntervalMapping>
float ApplyIntervalMapping(const float value, const TRuleWithIntervalMapping& extractionRule) {
    if (!extractionRule.HasIntervalMapping()) {
        return value;
    }

    const TIntervalMapping& mapping = extractionRule.GetIntervalMapping();
    Y_ENSURE(!mapping.GetPoints().empty());

    for (auto rIt = mapping.GetPoints().rbegin(); rIt != mapping.GetPoints().rend(); ++rIt) {
        // Points are sorted by threshold, check validation functions

        if (value >= rIt->GetThreshold()) {
            return rIt->GetValueAfter();
        }
    }

    return mapping.GetValueBefore();
}

float ExtractValue(const NBg::NProto::TAliceEntitiesCollectorResult& bgRuleResult,
                   const TRule::TEntitiesRule& extractionRule) {
    const TProtoStringType& entityType = extractionRule.GetEntityType();

    for (const NBg::NProto::TAliceEntity& entity : bgRuleResult.GetEntities()) {
        if (entity.GetType() == entityType) {
            return BoolToValue(true);
        }
    }

    return BoolToValue(false);
}

float ExtractValue(const NBg::NProto::TAliceGcDssmClassifierResult& bgRuleResult,
                   const TBinaryClassifierRule& extractionRule) {
    Y_ENSURE(extractionRule.GetClassifier() == TBinaryClassifierRule::ALICE_GC_DSSM,
             "inappropriate classificator type provided");
    return ApplyIntervalMapping(bgRuleResult.GetScore(), extractionRule);
}

float ExtractValue(const NBg::NProto::TAliceParsedFramesResult& bgRuleResult,
                   const TRule::TGranetFrameRule& extractionRule) {
    const TProtoStringType& frameName = extractionRule.GetFrameName();

    for (const auto [frame, source] : Zip(bgRuleResult.GetFrames(), bgRuleResult.GetSources())) {
        if (source == GRANET_SOURCE_LABEL && frame.GetName() == frameName) {
            return BoolToValue(true);
        }
    }

    return BoolToValue(false);
}

float ExtractValue(const NBg::NProto::TAliceWizDetectionResult& bgRuleResult,
                   const TRule::TAliceWizDetectionRule& extractionRule) {
    const TProtoStringType& featureName = extractionRule.GetFeatureName();

    for (const NBg::NProto::TModelResult& modelResult : bgRuleResult.GetModelResults()) {
        if (modelResult.GetName() == featureName) {
            return BoolToValue(modelResult.GetPassed());
        }
    }

    // need to log this situation somehow, config is invalid with current begemot rule
    return DEFAULT_FEATURE_VALUE;
}

float ExtractValue(const NBg::NProto::TAliceMultiIntentClassifierResult& bgRuleResult,
                   const TRule::TMultiIntentClassifierRule& extractionRule) {

    const NBg::NProto::TMultiClassifierResult* clfResult = MapFindPtr(bgRuleResult.GetClassifiersResult(), extractionRule.GetClassifier());
    if (clfResult == nullptr) {
        // need to log this situation somehow, config is invalid with current begemot rule
        return DEFAULT_FEATURE_VALUE;
    }

    const float* intentProbabilityPtr = MapFindPtr(clfResult->GetProbabilities(), extractionRule.GetIntentName());

    if (intentProbabilityPtr == nullptr) {
        // need to log this situation somehow, config is invalid with current begemot rule
        return DEFAULT_FEATURE_VALUE;
    }

    return ApplyIntervalMapping(*intentProbabilityPtr, extractionRule);
}

float ExtractValue(const NBg::NProto::TPornQueryResult& bgRuleResult) {
    return BoolToValue(bgRuleResult.GetIsPornoQuery());
}

template <typename TBgRuleResult>
inline bool IsApplicable(const TBgRuleResult* bgRuleResult, const TProtoRepeatedString& ruleExperiments,
                         const THashSet<TString>& requestExperiments) {
    if (bgRuleResult == nullptr) {
        return false;
    }

    return AllOf(ruleExperiments, [&requestExperiments](const auto& exp) { return requestExperiments.contains(exp); });
}

TMaybe<float> ExtractBinaryClassifierValue(const TBinaryClassifierRule& rule,
                                           const TProtoRepeatedString& ruleExperiments,
                                           const TFeatureAggregatorSources& sources,
                                           const THashSet<TString>& requestExperiments) {

    Y_ENSURE(TBinaryClassifierRule::EClassifier_IsValid(rule.GetClassifier()),
             "unknown enum value: " << static_cast<int>(rule.GetClassifier()));

    switch (rule.GetClassifier()) {
        case TBinaryClassifierRule::ALICE_GC_DSSM: {
            if (IsApplicable(sources.AliceGcDssmClassifier, ruleExperiments, requestExperiments)) {
                return ExtractValue(*sources.AliceGcDssmClassifier, rule);
            }
            break;
        }
        // default is not used to guarantee compile time failure if new enum is missing
        case TFeatureAggregatorConfig_TRule_TBinaryClassifierRule_EClassifier_TFeatureAggregatorConfig_TRule_TBinaryClassifierRule_EClassifier_INT_MAX_SENTINEL_DO_NOT_USE_:
        case TFeatureAggregatorConfig_TRule_TBinaryClassifierRule_EClassifier_TFeatureAggregatorConfig_TRule_TBinaryClassifierRule_EClassifier_INT_MIN_SENTINEL_DO_NOT_USE_: {
            // unreachable as enum value is valid
            Y_UNREACHABLE();
        }
    }

    return Nothing();
}

float Extract(const TFeature& feature, const TFeatureAggregatorSources& sources,
              const THashSet<TString>& requestExperiments) {
    for (const TRule& rule : feature.GetRules()) {
        switch (rule.GetSourceCase()) {
            case TRule::SourceCase::SOURCE_NOT_SET: {
                continue;
            }
            case TRule::SourceCase::kEntity: {
                if (IsApplicable(sources.AliceEntities, rule.GetExperiments(), requestExperiments)) {
                    return ExtractValue(*sources.AliceEntities, rule.GetEntity());
                }
                break;
            }
            case TRule::SourceCase::kGranetFrame: {
                if (IsApplicable(sources.AliceParsedFrames, rule.GetExperiments(), requestExperiments)) {
                    return ExtractValue(*sources.AliceParsedFrames, rule.GetGranetFrame());
                }
                break;
            }
            case TRule::SourceCase::kMultiIntentClassifier: {
                if (IsApplicable(sources.AliceMultiIntentClassifier, rule.GetExperiments(), requestExperiments)) {
                    return ExtractValue(*sources.AliceMultiIntentClassifier, rule.GetMultiIntentClassifier());
                }
                break;
            }
            case TRule::SourceCase::kBinaryClassifier: {
                const auto value = ExtractBinaryClassifierValue(rule.GetBinaryClassifier(), rule.GetExperiments(),
                                                                sources, requestExperiments);
                if (value.Defined()) {
                    return value.GetRef();
                }
                break;
            }
            case TRule::SourceCase::kWizDetection: {
                if (IsApplicable(sources.AliceWizDetection, rule.GetExperiments(), requestExperiments)) {
                    return ExtractValue(*sources.AliceWizDetection, rule.GetWizDetection());
                }
                break;
            }
            case TRule::SourceCase::kPornQuery: {
                if (IsApplicable(sources.PornQuery, rule.GetExperiments(), requestExperiments)) {
                    return ExtractValue(*sources.PornQuery);
                }
                break;
            }
        }
    }
    return DEFAULT_FEATURE_VALUE;
}

void StaticIsFreshPrefix(const TFeatureAggregatorConfig& staticConfig ,
                         const TFeatureAggregatorConfig& freshConfig) {

    Y_ENSURE_EX(staticConfig.GetFeatures().size() <= freshConfig.GetFeatures().size(), TValidationException() << "more static features than fresh features");

    for (const auto& [staticFeature, freshFeature] : Zip(staticConfig.GetFeatures(), freshConfig.GetFeatures())) {
        Y_ENSURE_EX(staticFeature.GetIndex() == freshFeature.GetIndex(),
            TValidationException() << "features doesn't have same indexes, static feature: " << staticFeature << ", fresh feature: " << freshFeature);
        Y_ENSURE_EX(staticFeature.GetName() == freshFeature.GetName(),
            TValidationException() << "features doesn't have same names, static feature: " << staticFeature << ", fresh feature: " << freshFeature);
    }
}

float Process(const TFeature& feature,
             const TFeatureAggregatorSources& sources,
             const THashSet<TString>& requestExperiments) {
    if (feature.GetIsDisabled()) {
         return DEFAULT_FEATURE_VALUE;
    }
    return Extract(feature, sources, requestExperiments);;
}


TProtoRepeatedFloat AggregateUnique(const TProtoRepeatedFeature& features,
                     const TFeatureAggregatorSources& sources,
                     const THashSet<TString>& requestExperiments) {

    int featureVectorSize;

    if (!features.empty()) {
        featureVectorSize = features.rbegin()->GetIndex() + 1;
    } else {
        featureVectorSize = 1;
    }

    TProtoRepeatedFloat featureVector;
    featureVector.Resize(featureVectorSize, DEFAULT_FEATURE_VALUE);

    for (const auto& feature : features) {
        featureVector[feature.GetIndex()] = Process(feature, sources, requestExperiments);
    }

    return featureVector;
}

TProtoRepeatedFloat AggregateBoth(const TProtoRepeatedFeature& staticFeatures,
                                  const TProtoRepeatedFeature& freshFeatures,
                                  const TProtoRepeatedString& forceForPrefixes,
                                  const TFeatureAggregatorSources& sources,
                                  const THashSet<TString>& requestExperiments,
                                  TVector<TString>& usedFreshFeatures) {
    int featureVectorSize;

    if (!freshFeatures.empty()) {
        featureVectorSize =  freshFeatures.rbegin()->GetIndex() + 1;
    } else {
        featureVectorSize = 1;
    }

    TProtoRepeatedFloat featureVector;
    featureVector.Resize(featureVectorSize, DEFAULT_FEATURE_VALUE);

    for (const auto& [i, freshFeature] : Enumerate(freshFeatures)) {
        const TString& featureName = freshFeature.GetName();
        const bool isFreshPrefix = AnyOf(forceForPrefixes,
                                      [&featureName](const TString& prefix) { return featureName.StartsWith(prefix); });

        if (isFreshPrefix) {
            usedFreshFeatures.push_back(freshFeature.GetName());
            featureVector[freshFeature.GetIndex()] = Process(freshFeature, sources, requestExperiments);
        } else if (static_cast<int>(i) < staticFeatures.size()) {
            featureVector[freshFeature.GetIndex()] = Process(staticFeatures[i], sources, requestExperiments);
        } else {
            // keep default value 
            continue;
        }
    }
    return featureVector;
}

} // namespace

TFeatureAggregator::TFeatureAggregator(const TFeatureAggregatorConfig& staticConfig) : StaticConfig(staticConfig) {}

TFeatureAggregator::TFeatureAggregator(const TFeatureAggregatorConfig& staticConfig,
                                       const TFeatureAggregatorConfig& freshConfig)
    : StaticConfig(staticConfig)
    , FreshConfig(freshConfig)
{
    StaticIsFreshPrefix(staticConfig, freshConfig);
}

TFeatureAggregatorResult TFeatureAggregator::Aggregate(const TFeatureAggregatorSources& sources,
                                                const THashSet<TString>& requestExperiments,
                                                const TFreshOptions& options) const {

    TProtoRepeatedFloat featureVector;
    TFeatureAggregatorResult result;

    if (!FreshConfig.Defined()) {
        featureVector = AggregateUnique(StaticConfig.GetFeatures(), sources, requestExperiments);
    } else if (options.GetForceEntireFresh()) {
        result.UsedEntireFresh = true;
        featureVector = AggregateUnique(FreshConfig.GetRef().GetFeatures(), sources, requestExperiments);
    } else if (options.GetForceForPrefixes().empty()) {
        featureVector = AggregateUnique(StaticConfig.GetFeatures(), sources, requestExperiments);
    } else {
        featureVector = AggregateBoth(
            StaticConfig.GetFeatures(), 
            FreshConfig.GetRef().GetFeatures(), 
            options.GetForceForPrefixes(), 
            sources, 
            requestExperiments,
            result.UsedFreshForFeatures
        );
    }

    *result.Features.MutableFeatures() = std::move(featureVector);

    return result;
}
} // namespace NAlice::NFeatureAggregator
