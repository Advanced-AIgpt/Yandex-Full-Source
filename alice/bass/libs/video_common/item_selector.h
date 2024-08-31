#pragma once

#include <alice/megamind/protos/common/device_state.pb.h>

#include <alice/library/logger/logadapter.h>
#include <alice/library/video_common/defs.h>

namespace NBASS::NVideoCommon {

struct TItemSelectionResult {
    /* SelectedIndex is the index of the best matching video (starting from 1), or -1, if there was no match */
    int Index = -1;
    float Confidence = -1.0;
};

TItemSelectionResult SelectVideoFromGallery(const NAlice::TDeviceState& deviceStateProto,
                                            const TString& videoText,
                                            const NAlice::TLogAdapter& logger);

} // namespace NBASS::NVideoCommon
