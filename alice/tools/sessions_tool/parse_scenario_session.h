#pragma once

#include <alice/megamind/library/session/protos/session.pb.h>

namespace NAlice {

TString GetScenarioSessionString(const TSessionProto& sessionProto, const TString& scenarioName, bool singleLineMode);

} // namespace NAlice
