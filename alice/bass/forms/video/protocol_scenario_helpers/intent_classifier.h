#pragma once

#include <alice/megamind/protos/scenarios/request.pb.h>

#include <library/cpp/scheme/scheme.h>

#include <alice/library/client/client_features.h>

namespace NVideoProtocol {

TStringBuf ChooseIntent(const NAlice::NScenarios::TScenarioRunRequest& request,
                        NAlice::TClientFeatures clientFeatures);

} // NAlice::NVideo
