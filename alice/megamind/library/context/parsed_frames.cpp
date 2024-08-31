#include "parsed_frames.h"

#include <alice/library/request/slot.h>
#include <google/protobuf/util/json_util.h>

namespace NAlice {

TParsedFramesResponse::TParsedFramesResponse(const ::NBg::NProto::TAliceParsedFramesResult& aliceParsedFramesResult) {
    Y_ENSURE(aliceParsedFramesResult.FramesSize() == aliceParsedFramesResult.SourcesSize());
    Y_ENSURE(aliceParsedFramesResult.FramesSize() == aliceParsedFramesResult.ConfidencesSize());

    Frames_.reserve(aliceParsedFramesResult.FramesSize());
    Sources_.reserve(aliceParsedFramesResult.SourcesSize());

    for (size_t i = 0; i < aliceParsedFramesResult.FramesSize(); ++i) {
        const auto& frame = Frames_.emplace_back(aliceParsedFramesResult.GetFrames(i));
        Sources_.emplace_back(aliceParsedFramesResult.GetSources(i));
        FrameConfidences_.emplace(frame.GetName(), aliceParsedFramesResult.GetConfidences(i));
    }
}

float TParsedFramesResponse::GetFrameConfidence(const TStringBuf frameName) const {
    // NOTE(a-square): Currently Begemot is storing confidences in a map indexed by frame
    // see GetFrameConfidence) name so we're justified in doing the same.
    return FrameConfidences_.Value(frameName, static_cast<float>(0.0));
}

const TVector<TSemanticFrame>& TParsedFramesResponse::GetFrames() const {
    return Frames_;
}

const TVector<TString>& TParsedFramesResponse::GetSources() const {
    return Sources_;
}

TMaybe<TSemanticFrame> TParsedFramesResponse::GetRecognizedAction() const {
    if (Sources_ && Sources_[0] == ACTION_RECOGNIZER_SOURCE_LABEL) {
        return Frames_[0];
    }
    return Nothing();
}

} // namespace NAlice
