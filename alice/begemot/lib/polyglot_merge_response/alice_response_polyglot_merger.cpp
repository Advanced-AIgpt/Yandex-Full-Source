#include "alice_response_polyglot_merger.h"
#include <util/string/builder.h>

namespace NAlice {

namespace {

using EMergeMode = TAliceResponsePolyglotMerger::EMergeMode;
using ESelectedFrameSource = TAliceResponsePolyglotMerger::ESelectedFrameSource;
using TAliceParsedFramesConfig = TAliceResponsePolyglotMergerConfig::TAliceParsedFramesConfig;

using TSelectFrameSourceCallback = std::function<ESelectedFrameSource(
    const TStringBuf frameName, const bool isNativeMatched, const bool isTranslatedMatched)>;

void ValidateAliceParsedFrames(const ::NBg::NProto::TAliceParsedFramesResult& aliceParsedFrames) {
    Y_ENSURE(aliceParsedFrames.FramesSize() == aliceParsedFrames.SourcesSize());
    Y_ENSURE(aliceParsedFrames.FramesSize() == aliceParsedFrames.ConfidencesSize());
}

TAliceResponsePolyglotMerger::TLogCallback BuildLogCallback(const bool isLogEnabled, ::NBg::NProto::TAlicePolyglotMergeResponseResult& result) {
    if (isLogEnabled) {
        return [&result](TString log) {
            result.AddLog(std::move(log));
        };
    }
    return {};
}

THashSet<TStringBuf> BuildFrameNameSetFromFrames(const ::NBg::NProto::TAliceParsedFramesResult& aliceParsedFrames) {
    auto result = THashSet<TStringBuf>(aliceParsedFrames.FramesSize());
    for (const auto& frame : aliceParsedFrames.GetFrames()) {
        result.emplace(frame.GetName());
    }
    return result;
}

void CopyFrameSourceConfidence(
    const ::NBg::NProto::TAliceParsedFramesResult& src,
    ::NBg::NProto::TAliceParsedFramesResult& dst,
    const size_t index)
{
    *dst.AddFrames() = src.GetFrames(index);
    dst.AddSources(src.GetSources(index));
    dst.AddConfidences(src.GetConfidences(index));
}

void MergeFramesSourcesConfidences(
    const ::NBg::NProto::TAliceParsedFramesResult& nativeAliceParsedFrames,
    const ::NBg::NProto::TAliceParsedFramesResult& translatedAliceParsedFrames,
    ::NBg::NProto::TAliceParsedFramesResult& result,
    TSelectFrameSourceCallback selectFrameSource,
    const TAliceResponsePolyglotMerger::TLogCallback log)
{
    const auto translatedFrameNames = BuildFrameNameSetFromFrames(translatedAliceParsedFrames);
    for (size_t nativeIndex = 0; nativeIndex < nativeAliceParsedFrames.FramesSize(); nativeIndex++) {
        const auto& frameName = nativeAliceParsedFrames.GetFrames(nativeIndex).GetName();
        const auto selectedFrameSource = selectFrameSource(frameName, true, translatedFrameNames.contains(frameName));
        if (selectedFrameSource == ESelectedFrameSource::Native) {
            CopyFrameSourceConfidence(nativeAliceParsedFrames, result, nativeIndex);
        }
        if (log) {
            log(TStringBuilder() << "Native frame '" << frameName << "' is merged with frame source = " << selectedFrameSource);
        }
    }

    const auto nativeFrameNames = BuildFrameNameSetFromFrames(nativeAliceParsedFrames);
    for (size_t translatedIndex = 0; translatedIndex < translatedAliceParsedFrames.FramesSize(); translatedIndex++) {
        const auto& frameName = translatedAliceParsedFrames.GetFrames(translatedIndex).GetName();
        const auto selectedFrameSource = selectFrameSource(frameName, nativeFrameNames.contains(frameName), true);
        if (selectedFrameSource == ESelectedFrameSource::Translated) {
            CopyFrameSourceConfidence(translatedAliceParsedFrames, result, translatedIndex);
        }
        if (log) {
            log(TStringBuilder() << "Translated frame '" << frameName << "' is merged with frame source = " << selectedFrameSource);
        }
    }
}

template <class T>
THashSet<TStringBuf> BuildFrameNameSetForDebugItems(const ::google::protobuf::RepeatedPtrField<T>& debugItems) {
    auto result = THashSet<TStringBuf>(debugItems.size());
    for (const auto& debugItem : debugItems) {
        result.emplace(debugItem.GetFrameName());
    }
    return result;
}

template <class T>
void MergeDebugInfo(
    const ::google::protobuf::RepeatedPtrField<T>& nativeDebugItems,
    const ::google::protobuf::RepeatedPtrField<T>& translatedDebugItems,
    ::google::protobuf::RepeatedPtrField<T>& resultDebugItems,
    TSelectFrameSourceCallback selectFrameSource)
{
    const auto translatedFrameNames = BuildFrameNameSetForDebugItems(translatedDebugItems);
    for (const auto& nativeDebugItem : nativeDebugItems) {
        const auto& frameName = nativeDebugItem.GetFrameName();
        if (selectFrameSource(frameName, true, translatedFrameNames.contains(frameName)) == ESelectedFrameSource::Native) {
            *resultDebugItems.Add() = nativeDebugItem;
        }
    }

    const auto nativeFrameNames = BuildFrameNameSetForDebugItems(nativeDebugItems);
    for (const auto& translatedDebugItem : translatedDebugItems) {
        const auto& frameName = translatedDebugItem.GetFrameName();
        if (selectFrameSource(frameName, nativeFrameNames.contains(frameName), true) == ESelectedFrameSource::Translated) {
            *resultDebugItems.Add() = translatedDebugItem;
        }
    }
}

ESelectedFrameSource SelectFrameSourceNativeOnly(const bool isNativeMatched, const bool /*isTranslatedMatched*/) {
    return isNativeMatched ? ESelectedFrameSource::Native : ESelectedFrameSource::Skip;
}

ESelectedFrameSource SelectFrameSourceTranslatedOnly(const bool /*isNativeMatched*/, const bool isTranslatedMatched) {
    return isTranslatedMatched ? ESelectedFrameSource::Translated : ESelectedFrameSource::Skip;
}

ESelectedFrameSource SelectFrameSourceNativeOrTranslated(const bool isNativeMatched, const bool isTranslatedMatched) {
    if (isNativeMatched) {
        return ESelectedFrameSource::Native;
    }
    if (isTranslatedMatched) {
        return ESelectedFrameSource::Translated;
    }
    return ESelectedFrameSource::Skip;
}

ESelectedFrameSource SelectFrameSourceTranslatedOrNative(const bool isNativeMatched, const bool isTranslatedMatched) {
    if (isTranslatedMatched) {
        return ESelectedFrameSource::Translated;
    }
    if (isNativeMatched) {
        return ESelectedFrameSource::Native;
    }
    return ESelectedFrameSource::Skip;
}

} // namespace

TAliceResponsePolyglotMerger::TAliceResponsePolyglotMerger(
    const TAliceResponsePolyglotMergerConfig& config,
    const TFrameAggregatorConfig& frameAggregatorConfig,
    TMaybe<EMergeMode> forceFrameMergeMode,
    bool isLogEnabled)
    : ForceFrameMergeMode_(std::move(forceFrameMergeMode))
    , IsLogEnabled_(isLogEnabled)
{
    for (const auto& aliceParsedFrame : config.GetAliceParsedFrames()) {
        FrameNameToMergeModeMap_.insert_or_assign(aliceParsedFrame.GetName(), aliceParsedFrame.GetMergeMode());
    }
    for (const auto& frame : frameAggregatorConfig.GetFrames()) {
        NativeFrameNames_.insert(frame.GetName());
    }
}

::NBg::NProto::TAliceParsedFramesResult TAliceResponsePolyglotMerger::MergeAliceParsedFrames(
    const ::NBg::NProto::TAliceParsedFramesResult& nativeAliceParsedFrames,
    const ::NBg::NProto::TAliceParsedFramesResult& translatedAliceParsedFrames,
    const TLogCallback log) const
{
    ValidateAliceParsedFrames(nativeAliceParsedFrames);
    ValidateAliceParsedFrames(translatedAliceParsedFrames);

    const TSelectFrameSourceCallback selectFrameSource = [this](const TStringBuf frameName, const bool isNativeMatched, const bool isTranslatedMatched) {
        return this->SelectFrameSource(frameName, isNativeMatched, isTranslatedMatched);
    };

    ::NBg::NProto::TAliceParsedFramesResult result;

    MergeFramesSourcesConfidences(nativeAliceParsedFrames, translatedAliceParsedFrames, result, selectFrameSource, log);

    if (nativeAliceParsedFrames.GetDebugInfo().MatchedRulesSize() || translatedAliceParsedFrames.GetDebugInfo().MatchedRulesSize()) {
        MergeDebugInfo(
            nativeAliceParsedFrames.GetDebugInfo().GetMatchedRules(),
            translatedAliceParsedFrames.GetDebugInfo().GetMatchedRules(),
            *result.MutableDebugInfo()->MutableMatchedRules(),
            selectFrameSource);
    }

    if (nativeAliceParsedFrames.GetDebugInfo().TriggeredClassifiersByLoggingThresholdSize() || translatedAliceParsedFrames.GetDebugInfo().TriggeredClassifiersByLoggingThresholdSize()) {
        MergeDebugInfo(
            nativeAliceParsedFrames.GetDebugInfo().GetTriggeredClassifiersByLoggingThreshold(),
            translatedAliceParsedFrames.GetDebugInfo().GetTriggeredClassifiersByLoggingThreshold(),
            *result.MutableDebugInfo()->MutableTriggeredClassifiersByLoggingThreshold(),
            selectFrameSource);
    }

    return result;
}

ESelectedFrameSource TAliceResponsePolyglotMerger::SelectFrameSource(
    const TStringBuf frameName, const bool isNativeMatched, const bool isTranslatedMatched) const
{
    EMergeMode mergeMode;
    if (ForceFrameMergeMode_) {
        mergeMode = ForceFrameMergeMode_.GetRef();
    } else if (const auto* configMergeMode = FrameNameToMergeModeMap_.FindPtr(frameName)) {
        mergeMode = *configMergeMode;
    } else {
        mergeMode = TAliceParsedFramesConfig::Default;
    }

    switch (mergeMode) {
        case TAliceParsedFramesConfig::NativeOnly:
            return SelectFrameSourceNativeOnly(isNativeMatched, isTranslatedMatched);
        case TAliceParsedFramesConfig::TranslatedOnly:
            return SelectFrameSourceTranslatedOnly(isNativeMatched, isTranslatedMatched);
        case TAliceParsedFramesConfig::NativeOrTranslated:
            return SelectFrameSourceNativeOrTranslated(isNativeMatched, isTranslatedMatched);
        case TAliceParsedFramesConfig::TranslatedOrNative:
            return SelectFrameSourceTranslatedOrNative(isNativeMatched, isTranslatedMatched);
        case TAliceParsedFramesConfig::Default:
        case TAliceParsedFramesConfig::NativeOnlyIfExists:
            return NativeFrameNames_.contains(frameName)
                ? SelectFrameSourceNativeOnly(isNativeMatched, isTranslatedMatched)
                : SelectFrameSourceNativeOrTranslated(isNativeMatched, isTranslatedMatched);
    };
    return ESelectedFrameSource::Skip;
}

void TAliceResponsePolyglotMerger::MergeAliceNormalizer(
    const ::NBg::NProto::TAliceResponseResult& nativeAliceResponse,
    const ::NBg::NProto::TAliceResponseResult& translatedAliceResponse,
    ::NBg::NProto::TAlicePolyglotMergeResponseResult& result) const
{
    if (nativeAliceResponse.HasAliceNormalizer()) {
        *result.MutableAliceResponse()->MutableAliceNormalizer() = nativeAliceResponse.GetAliceNormalizer();
    } else {
        result.MutableAliceResponse()->ClearAliceNormalizer();
    }

    if (translatedAliceResponse.HasAliceNormalizer()) {
        *result.MutableTranslatedResponse()->MutableAliceNormalizer() = translatedAliceResponse.GetAliceNormalizer();
    }
}

void TAliceResponsePolyglotMerger::MergeAliceResponses(
    const ::NBg::NProto::TAliceResponseResult& nativeAliceResponse,
    const ::NBg::NProto::TAliceResponseResult* translatedAliceResponsePtr,
    ::NBg::NProto::TAlicePolyglotMergeResponseResult& result) const
{
    const auto log = BuildLogCallback(IsLogEnabled_, result);
    if (log && ForceFrameMergeMode_) {
        log(TStringBuilder() << "Using ForceFrameMergeMode = " << TAliceResponsePolyglotMergerConfig::TAliceParsedFramesConfig::EMergeMode_Name(ForceFrameMergeMode_.GetRef()));
    }

    const auto& translatedAliceResponse = translatedAliceResponsePtr
        ? *translatedAliceResponsePtr
        : ::NBg::NProto::TAliceResponseResult::default_instance();

    // maybe we should always use native response, but now we grab featues, fixlists, etc from russian begemot
    *result.MutableAliceResponse() = translatedAliceResponsePtr
        ? translatedAliceResponse
        : nativeAliceResponse;

    if (nativeAliceResponse.HasAliceParsedFrames() || translatedAliceResponse.HasAliceParsedFrames()) {
        *result.MutableAliceResponse()->MutableAliceParsedFrames() = MergeAliceParsedFrames(
            nativeAliceResponse.GetAliceParsedFrames(),
            translatedAliceResponse.GetAliceParsedFrames(),
            log);
    }

    if (nativeAliceResponse.HasAliceNormalizer() || translatedAliceResponse.HasAliceNormalizer()) {
        MergeAliceNormalizer(nativeAliceResponse, translatedAliceResponse, result);
    }
}

}
