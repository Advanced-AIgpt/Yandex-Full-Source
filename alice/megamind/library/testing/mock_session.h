#pragma once

#include <alice/megamind/library/session/session.h>

#include <alice/megamind/protos/analytics/megamind_analytics_info.pb.h>
#include <alice/megamind/protos/quality_storage/storage.pb.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice {

class TMockSession : public ISession {
public:
    MOCK_METHOD(const TString&, GetPreviousScenarioName, (), (const, override));
    MOCK_METHOD(TMaybe<TResponseBuilderProto>, GetScenarioResponseBuilder, (), (const, override));
    MOCK_METHOD(TDialogHistory, GetDialogHistory, (), (const, override));
    MOCK_METHOD(TMaybe<NScenarios::TLayout>, GetLayout, (), (const, override));
    MOCK_METHOD(TMaybe<TSemanticFrame>, GetResponseFrame, (), (const, override));
    MOCK_METHOD(::google::protobuf::RepeatedPtrField<TClientEntity>, GetResponseEntities, (), (const, override));
    MOCK_METHOD(TMaybe<TSessionProto::TProtocolInfo>, GetProtocolInfo, (), (const, override));
    MOCK_METHOD(TMaybe<NMegamind::TMegamindAnalyticsInfo>, GetMegamindAnalyticsInfo, (), (const, override));
    MOCK_METHOD(TMaybe<TQualityStorage>, GetQualityStorage, (), (const, override));
    MOCK_METHOD(const TString&, GetIntentName, (), (const, override));
    MOCK_METHOD(TMaybe<NMegamind::TModifiersStorage>, GetModifiersStorage, (), (const, override));
    MOCK_METHOD(TProactivityAnswer, GetProactivityRecommendations, (), (const, override));
    MOCK_METHOD((::google::protobuf::Map<TString, NScenarios::TFrameAction>), GetActions, (), (const, override));
    MOCK_METHOD(TMaybe<TSemanticFrame::TSlot>, GetRequestedSlot, (), (const, override));
    MOCK_METHOD(ISessionBuilder*, GetUpdaterMockMethod, (), (const));
    MOCK_METHOD(ISessionBuilder*, GetBuilderMockMethod, (), (const));
    MOCK_METHOD(const TSessionProto::TScenarioSession&, GetScenarioSession, (const TString&), (const, override));
    MOCK_METHOD((const ::google::protobuf::Map<TString, TSessionProto::TScenarioSession>&), GetScenarioSessions, (), (const, override));
    MOCK_METHOD(const TSessionProto::TScenarioSession&, GetPreviousScenarioSession, (), (const, override));
    MOCK_METHOD(TMaybe<NAlice::TGcMemoryState>, GetGcMemoryState, (), (const, override));
    MOCK_METHOD(bool, GetRequestIsExpected, (), (const, override));
    MOCK_METHOD(const NMegamind::TStackEngineCore&, GetStackEngineCore, (), (const, override));
    MOCK_METHOD(TMaybe<NScenarios::TInput>, GetInput, (), (const, override));
    MOCK_METHOD(const TString&, GetPreviousProductScenarioName, (), (const, override));
    MOCK_METHOD(ui64, GetLastWhisperTimeMs, (), (const, override));

    MOCK_METHOD(const TState&, GetScenarioState, (), (const));
    MOCK_METHOD(ui32, GetConsequentIrrelevantResponseCount, (), (const));
    MOCK_METHOD(i32, GetActivityTurn, (), (const));

    THolder<ISessionBuilder> GetUpdater() const & override {
        return THolder<ISessionBuilder>(GetUpdaterMockMethod());
    }

    THolder<ISessionBuilder> GetUpdater() && override {
        return THolder<ISessionBuilder>(GetUpdaterMockMethod());
    }

    THolder<ISessionBuilder> CreateBuilder() const override {
        return THolder<ISessionBuilder>(GetBuilderMockMethod());
    }

    // Throws an exception in case of failure.
    MOCK_METHOD(TString, Serialize, (), (const, override));

    MOCK_METHOD(const TSessionProto&, Proto, (), (const, override));
};


} // namespace NAlice
