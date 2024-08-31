#pragma once

#include <search/begemot/rules/alice/parsed_frames/proto/alice_parsed_frames.pb.h>

#include <alice/library/logger/logger.h>
#include <alice/megamind/protos/common/frame.pb.h>
#include <library/cpp/json/json_value.h>
#include <util/generic/maybe.h>

namespace NAlice {

inline constexpr TStringBuf ACTION_RECOGNIZER_SOURCE_LABEL = "AliceActionRecognizer";
inline constexpr TStringBuf FIXLIST_SOURCE_LABEL = "AliceFixlist";
inline constexpr TStringBuf FRAME_FILLER_SOURCE_LABEL = "AliceFrameFiller";
inline constexpr TStringBuf TAGGER_SOURCE_LABEL = "AliceTagger";
inline constexpr TStringBuf GRANET_SOURCE_LABEL = "Granet";
inline constexpr TStringBuf MICROINTENTS_SOURCE_LABEL = "AliceMicrointents";

class TParsedFramesResponse {
public:
    explicit TParsedFramesResponse() = default;
    explicit TParsedFramesResponse(const ::NBg::NProto::TAliceParsedFramesResult& aliceParsedFramesResult);

    float GetFrameConfidence(const TStringBuf frameName) const;
    const TVector<TSemanticFrame>& GetFrames() const;
    const TVector<TString>& GetSources() const;
    TMaybe<TSemanticFrame> GetRecognizedAction() const;
private:
    TVector<TSemanticFrame> Frames_;
    TVector<TString> Sources_;
    THashMap<TString, float> FrameConfidences_;
};

} // namespace NAlice
