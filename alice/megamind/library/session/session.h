#pragma once

#include "dialog_history.h"

#include <alice/megamind/library/session/protos/session.pb.h>
#include <alice/megamind/library/session/protos/state.pb.h>
#include <alice/megamind/library/stack_engine/protos/stack_engine.pb.h>

#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/common/gc_memory_state.pb.h>
#include <alice/megamind/protos/modifiers/modifiers.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>
#include <dj/services/alisa_skills/server/proto/client/proactivity_response.pb.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice {

class ISessionBuilder;
class TQualityStorage;

using TProactivityAnswer = TVector<NDJ::NAS::TProactivityRecommendation>;

class ISession {
public:
    virtual ~ISession() = default;

    virtual const TString& GetPreviousScenarioName() const = 0;
    virtual const TString& GetPreviousProductScenarioName() const = 0;
    virtual TMaybe<TResponseBuilderProto> GetScenarioResponseBuilder() const = 0;
    virtual TDialogHistory GetDialogHistory() const = 0;
    virtual TMaybe<NScenarios::TLayout> GetLayout() const = 0;
    virtual TMaybe<TSemanticFrame> GetResponseFrame() const = 0;
    virtual ::google::protobuf::RepeatedPtrField<TClientEntity> GetResponseEntities() const = 0;
    virtual TMaybe<TSessionProto::TProtocolInfo> GetProtocolInfo() const = 0;
    virtual TMaybe<NMegamind::TMegamindAnalyticsInfo> GetMegamindAnalyticsInfo() const = 0;
    virtual TMaybe<TQualityStorage> GetQualityStorage() const = 0;
    virtual const TString& GetIntentName() const = 0;
    virtual TMaybe<NMegamind::TModifiersStorage> GetModifiersStorage() const = 0;
    virtual TProactivityAnswer GetProactivityRecommendations() const = 0;
    virtual ::google::protobuf::Map<TString, NScenarios::TFrameAction> GetActions() const = 0;
    virtual const TSessionProto::TScenarioSession& GetScenarioSession(const TString& name) const = 0;
    virtual const ::google::protobuf::Map<TString, TSessionProto::TScenarioSession>& GetScenarioSessions() const = 0;
    virtual const TSessionProto::TScenarioSession& GetPreviousScenarioSession() const = 0;
    virtual TMaybe<TSemanticFrame::TSlot> GetRequestedSlot() const = 0;
    virtual TMaybe<NAlice::TGcMemoryState> GetGcMemoryState() const = 0;
    virtual bool GetRequestIsExpected() const = 0;
    virtual const NMegamind::TStackEngineCore& GetStackEngineCore() const = 0;
    virtual TMaybe<NScenarios::TInput> GetInput() const = 0;

    virtual THolder<ISessionBuilder> GetUpdater() && = 0;
    virtual THolder<ISessionBuilder> GetUpdater() const & = 0;
    // inherits only speechkit session
    virtual THolder<ISessionBuilder> CreateBuilder() const = 0;
    virtual ui64 GetLastWhisperTimeMs() const = 0;

    // Throws an exception in case of failure.
    virtual TString Serialize() const = 0;

    virtual const TSessionProto& Proto() const = 0;
};

using TScenarioSessionModifier = std::function<void(const TString&, TSessionProto::TScenarioSession&)>;

class ISessionBuilder {
public:
    virtual ~ISessionBuilder() = default;

    virtual ISessionBuilder& SetPreviousScenarioName(const TString& scenarioName) = 0;
    virtual ISessionBuilder& SetPreviousProductScenarioName(const TString& scenarioName) = 0;
    virtual ISessionBuilder& SetScenarioResponseBuilder(const TMaybe<TResponseBuilderProto>& responseBuilder) = 0;
    virtual ISessionBuilder& SetDialogHistory(const TDialogHistory& dialogHistory) = 0;
    virtual ISessionBuilder& SetLayout(const NScenarios::TLayout& layout) = 0;
    virtual ISessionBuilder& SetResponseFrame(const TMaybe<TSemanticFrame>& frame) = 0;
    virtual ISessionBuilder& SetResponseEntities(const ::google::protobuf::RepeatedPtrField<TClientEntity>& entities) = 0;
    virtual ISessionBuilder& SetProtocolInfo(const TMaybe<TSessionProto::TProtocolInfo>& protocolInfo) = 0;
    virtual ISessionBuilder& SetMegamindAnalyticsInfo(const TMaybe<NMegamind::TMegamindAnalyticsInfo>& analyticsInfo) = 0;
    virtual ISessionBuilder& SetQualityStorage(const TMaybe<TQualityStorage>& storage) = 0;
    virtual ISessionBuilder& SetIntentName(const TString& intentName) = 0;
    virtual ISessionBuilder& SetModifiersStorage(const TMaybe<NMegamind::TModifiersStorage>& modifiers) = 0;
    virtual ISessionBuilder& SetProactivityRecommendations(const TProactivityAnswer& rec) = 0;
    virtual ISessionBuilder& SetActions(const ::google::protobuf::Map<TString, NScenarios::TFrameAction>& actions) = 0;
    virtual ISessionBuilder& SetGcMemoryState(const TMaybe<NAlice::TGcMemoryState>& gcMemoryState) = 0;
    virtual ISessionBuilder& SetRequestIsExpected(bool requestIsExpected) = 0;
    virtual ISessionBuilder& SetScenarioSession(const TString& name, const TSessionProto::TScenarioSession& session) = 0;
    virtual ISessionBuilder& ModifyScenarioSessions(const TScenarioSessionModifier& modifier) = 0;
    virtual ISessionBuilder& SetScenarioSessions(const ::google::protobuf::Map<TString, TSessionProto::TScenarioSession>& scenarioSessions) = 0;
    virtual ISessionBuilder& SetStackEngineCore(const NMegamind::TStackEngineCore& core, bool invalidate = true) = 0;
    virtual ISessionBuilder& SetInput(const TMaybe<NScenarios::TInput>& input) = 0;
    virtual ISessionBuilder& SetLastWhisperTimeMs(const ui64 whisperTimeMs) = 0;
    virtual THolder<ISessionBuilder> Copy() const = 0;

    // Builder is in inconsistent state and shouldn't be used after Build is called
    virtual THolder<ISession> Build() = 0;
};

THolder<ISessionBuilder> MakeSessionBuilder();

THolder<ISession> MakeMegamindSession(TSessionProto&& megaminSession);
// Throws an exception in case of failure.
THolder<ISession> DeserializeSession(const TString& serialized);

NJson::TJsonValue ParseRawJsonSession(const TString& serialized);
THolder<ISession> DeserializeMegamindSessionProto(const NJson::TJsonValue& speechkitSession);
TSessionProto::TScenarioSession NewScenarioSession(const TState& state);

} // namespace NAlice
