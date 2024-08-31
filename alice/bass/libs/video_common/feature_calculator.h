#pragma once

#include <alice/megamind/protos/common/device_state.pb.h>

#include <alice/library/logger/logadapter.h>
#include <alice/library/video_common/protos/features.pb.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/scheme/scheme.h>

namespace NBASS::NVideoCommon {

void CalculateFeaturesAtStart(const TStringBuf intentType,
                              const TMaybe<TString>& searchText,
                              NAlice::NVideoCommon::TVideoFeatures& features,
                              const NAlice::TDeviceState& deviceState,
                              float selectorConfidenceByName,
                              float selectorConfidenceByNumber,
                              const NAlice::TLogAdapter& logger);

void CalculateFeaturesAtFinish(const TStringBuf intentType,
                               const TMaybe<TString>& searchText,
                               NAlice::NVideoCommon::TVideoFeatures& features,
                               const NSc::TValue& bassResponse,
                               bool isFinished,
                               ELanguage lang,
                               const NAlice::TLogAdapter& logger);

} // namespace NBASS::NVideoCommon
