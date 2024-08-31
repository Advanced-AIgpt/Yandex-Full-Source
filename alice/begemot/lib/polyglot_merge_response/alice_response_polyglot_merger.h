#pragma once

#include <alice/begemot/lib/frame_aggregator/proto/config.pb.h>
#include <alice/begemot/lib/polyglot_merge_response/proto/config.pb.h>

#include <search/begemot/rules/alice/parsed_frames/proto/alice_parsed_frames.pb.h>
#include <search/begemot/rules/alice/polyglot_merge_response/proto/alice_polyglot_merge_response.pb.h>
#include <search/begemot/rules/alice/response/proto/alice_response.pb.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>


namespace NAlice {

class TAliceResponsePolyglotMerger {
public:
    using EMergeMode = TAliceResponsePolyglotMergerConfig::TAliceParsedFramesConfig::EMergeMode;
    using TLogCallback = std::function<void(TString)>;
public:
    enum class ESelectedFrameSource {
        Skip,
        Native,
        Translated,
    };
public:
    explicit TAliceResponsePolyglotMerger(
        const TAliceResponsePolyglotMergerConfig& config,
        const TFrameAggregatorConfig& frameAggregatorConfig,
        TMaybe<EMergeMode> forceFrameMergeMode,
        bool isLogEnabled);

    void MergeAliceResponses(
        const ::NBg::NProto::TAliceResponseResult& nativeAliceResponse,
        const ::NBg::NProto::TAliceResponseResult* translatedAliceResponsePtr,
        ::NBg::NProto::TAlicePolyglotMergeResponseResult& result) const;
private:
    ::NBg::NProto::TAliceParsedFramesResult MergeAliceParsedFrames(
        const ::NBg::NProto::TAliceParsedFramesResult& nativeAliceParsedFrames,
        const ::NBg::NProto::TAliceParsedFramesResult& translatedAliceParsedFrames,
        const TLogCallback log) const;

    void MergeAliceNormalizer(
        const ::NBg::NProto::TAliceResponseResult& nativeAliceResponse,
        const ::NBg::NProto::TAliceResponseResult& translatedAliceResponse,
        ::NBg::NProto::TAlicePolyglotMergeResponseResult& result) const;

    ESelectedFrameSource SelectFrameSource(
        const TStringBuf frameName, const bool isNativeMatched, const bool isTranslatedMatched) const;
private:
    THashMap<TString, EMergeMode> FrameNameToMergeModeMap_;
    THashSet<TString> NativeFrameNames_;
    TMaybe<EMergeMode> ForceFrameMergeMode_;
    bool IsLogEnabled_ = false;
};

}
