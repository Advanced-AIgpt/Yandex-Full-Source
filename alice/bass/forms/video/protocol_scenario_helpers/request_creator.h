#pragma once

#include <alice/library/video_common/protos/features.pb.h>

#include <alice/megamind/protos/scenarios/request.pb.h>

#include <library/cpp/scheme/scheme.h>

#include <alice/bass/util/error.h>

namespace NVideoProtocol {

NBASS::TResultValue CreateBassRunVideoRequest(const NAlice::NScenarios::TScenarioRunRequest& request,
                                              NSc::TValue& bassRunRequest,
                                              NAlice::NVideoCommon::TVideoFeatures& features,
                                              TStringBuf& intentType, TMaybe<TString>& searchText);
NBASS::TResultValue CreateBassApplyVideoRequest(const NAlice::NScenarios::TScenarioApplyRequest& request,
                                                NSc::TValue& bassApplyRequest);

} // NVideoProtocol
