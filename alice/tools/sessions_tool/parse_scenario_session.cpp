#include "parse_scenario_session.h"

#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>
#include <alice/library/proto/proto.h>
#include <alice/megamind/library/session/protos/state.pb.h>
#include <alice/vins/api/vins_api/speechkit/connectors/protocol/protos/state.pb.h>

#include <google/protobuf/any.pb.h>

#include <util/generic/maybe.h>

namespace NAlice {
namespace {

const TString GENERAL_CONVERSATION_SCENARIO_NAME = "GeneralConversation";
const TString VINS_SCENARIO_NAME = "Vins";

TMaybe<TSessionProto::TScenarioSession> GetSessionByName(const TSessionProto& sessionProto, const TString& name) {
    const auto& sessions = sessionProto.GetScenarioSessions();
    if (const auto& it = sessions.find(name); it != sessions.end()) {
        return it->second;
    }

    return Nothing();
}

TString ParseGeneralConversationState(const TSessionProto::TScenarioSession& scenarioSession, bool singleLineMode) {
    NAlice::NHollywood::NGeneralConversation::TSessionState sessionState;
    scenarioSession.GetState().GetState().UnpackTo(&sessionState);

    return NAlice::SerializeProtoText(sessionState, singleLineMode);
}

TString ParseVinsState(const TSessionProto::TScenarioSession& scenarioSession, bool /* singleLineMode */) {
    NAlice::NProtoVins::TState state;
    scenarioSession.GetState().GetState().UnpackTo(&state);
    return state.GetSession();
}

} // namespace

TString GetScenarioSessionString(const TSessionProto& sessionProto, const TString& scenarioName, bool singleLineMode) {
    auto scenarioSession = GetSessionByName(sessionProto, scenarioName);
    if (!scenarioSession) {
        return "Scenario not found";
    }

    if (scenarioName == GENERAL_CONVERSATION_SCENARIO_NAME) {
        return ParseGeneralConversationState(scenarioSession.GetRef(), singleLineMode);
    }

    if (scenarioName == VINS_SCENARIO_NAME) {
        return ParseVinsState(scenarioSession.GetRef(), singleLineMode);
    }

    return "Don't know how to parse scenario";
}

} // namespace NAlice
