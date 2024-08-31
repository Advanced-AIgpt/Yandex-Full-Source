#include "utils.h"

#include <library/cpp/iterator/mapped.h>

#include <util/string/join.h>


namespace NAlice {

TString GetFrameNameListString(const TVector<TSemanticFrame>& frames) {
    if (frames.empty()) {
        return TString{"-"};
    }
    auto range = MakeMappedRange(frames, [](const TSemanticFrame& frame) {
        return frame.GetName();
    });
    return JoinRange(/* delim= */ TStringBuf(", "), range.begin(), range.end());
}

} // namespace NAlice
