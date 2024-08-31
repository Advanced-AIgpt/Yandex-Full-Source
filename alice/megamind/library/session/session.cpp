#include "session.h"

#include <alice/megamind/library/scenarios/defs/names.h>
#include <alice/megamind/library/util/slot.h>

#include <alice/megamind/protos/analytics/megamind_analytics_info.pb.h>
#include <alice/megamind/protos/quality_storage/storage.pb.h>

#include <alice/library/proto/protobuf.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/datetime/base.h>
#include <util/stream/str.h>
#include <util/stream/zlib.h>

#include <utility>

namespace NAlice {

namespace {

constexpr TStringBuf FIELD_MEGAMIND = "__megamind__";

class TSessionBuilder;

void ValidateProto(const TSessionProto& proto) {
    if (!proto.HasPreviousScenarioName()) {
        ythrow yexception() << "No scenario name in the session proto";
    }
    const auto& scenarioSessions = proto.GetScenarioSessions();
    if (const auto& it = scenarioSessions.find(proto.GetPreviousScenarioName());
        it == scenarioSessions.end() || !it->second.HasState())
    {
        ythrow yexception() << "No scenario state in the session proto";
    }
}

class TSession : public ISession {
public:
    TSession(const TSessionProto& proto) {
        ValidateProto(proto);
        Proto_ = proto;
        ValidateProto(Proto_);
    }

    TSession(TSessionProto&& proto) {
        ValidateProto(proto);
        Proto_ = std::move(proto);
        ValidateProto(Proto_);
    }

    const TString& GetPreviousScenarioName() const override {
        Y_ASSERT(Proto_.HasPreviousScenarioName());
        return Proto_.GetPreviousScenarioName();
    }

    const TString& GetPreviousProductScenarioName() const override {
        return Proto_.GetPreviousProductScenarioName();
    }

    TMaybe<TResponseBuilderProto> GetScenarioResponseBuilder() const override {
        if (Proto_.HasScenarioResponseBuilder())
            return Proto_.GetScenarioResponseBuilder();
        return Nothing();
    }

    TDialogHistory GetDialogHistory() const override {
        if (Proto_.HasDialogHistoryInfo()) {
            TDialogHistory dialogHistory;
            const auto& dialogHistoryInfo = Proto_.GetDialogHistoryInfo();
            for (const auto& dialogTurn : dialogHistoryInfo.GetDialogTurns()) {
                dialogHistory.PushDialogTurn(
                    {
                        dialogTurn.GetRequest(),
                        dialogTurn.GetRewrittenRequest(),
                        dialogTurn.GetResponse(),
                        dialogTurn.GetScenarioName(),
                        dialogTurn.GetServerTimeMs(),
                        dialogTurn.GetClientTimeMs()
                    }
                );
            }

            return dialogHistory;
        }

        const auto& dialogHistoryProto = Proto_.GetDialogHistory();
        auto dialogHistoryIter = dialogHistoryProto.begin();
        const auto& rewrittenRequestHistoryProto = Proto_.GetRewrittenRequestHistory();
        auto rewrittenRequestHistoryIter = rewrittenRequestHistoryProto.begin();
        TDialogHistory dialogHistory;
        while (dialogHistoryIter != dialogHistoryProto.end() || rewrittenRequestHistoryIter != rewrittenRequestHistoryProto.end()) {
            TString request;
            TString rewrittenRequest;
            TString response;
            if (dialogHistoryIter != dialogHistoryProto.end()) {
                request = *dialogHistoryIter;
                ++dialogHistoryIter;
            }
            if (dialogHistoryIter != dialogHistoryProto.end()) {
                response = *dialogHistoryIter;
                ++dialogHistoryIter;
            }
            if (rewrittenRequestHistoryIter != rewrittenRequestHistoryProto.end()) {
                rewrittenRequest = *rewrittenRequestHistoryIter;
                ++rewrittenRequestHistoryIter;
            }
            dialogHistory.PushDialogTurn({request, rewrittenRequest, response, /* ScenarioName= */ "",
                                          /* ServerTimeMs= */ 0, /* ClientTimeMs= */0});
        }

        return dialogHistory;
    }

    TMaybe<NScenarios::TLayout> GetLayout() const override {
        if (Proto_.HasLayout()) {
            return Proto_.GetLayout();
        }
        return Nothing();
    }

    TMaybe<TSemanticFrame> GetResponseFrame() const override {
        if (Proto_.HasResponseFrame()) {
            return Proto_.GetResponseFrame();
        }
        return Nothing();
    }

    ::google::protobuf::RepeatedPtrField<TClientEntity> GetResponseEntities() const override {
        return Proto_.GetResponseEntities();
    }

    TMaybe<TSessionProto::TProtocolInfo> GetProtocolInfo() const override {
        if (Proto_.HasProtocolInfo()) {
            return Proto_.GetProtocolInfo();
        }
        return Nothing();
    }

    TMaybe<NMegamind::TMegamindAnalyticsInfo> GetMegamindAnalyticsInfo() const override {
        if (Proto_.HasMegamindAnalyticsInfo()) {
            return Proto_.GetMegamindAnalyticsInfo();
        }
        return Nothing();
    }

    TMaybe<TQualityStorage> GetQualityStorage() const override {
        if (Proto_.HasQualityStorage()) {
            return Proto_.GetQualityStorage();
        }
        return Nothing();
    }

    ui64 GetLastWhisperTimeMs() const override {
        return Proto_.GetLastWhisperServerTimeMs();
    }

    const TString& GetIntentName() const override {
        return Proto_.GetIntentName();
    }

    TMaybe<NMegamind::TModifiersStorage> GetModifiersStorage() const override {
        if (Proto_.HasModifiersStorage()) {
            return Proto_.GetModifiersStorage();
        }
        return Nothing();
    }

    TProactivityAnswer GetProactivityRecommendations() const override {
        return {Proto_.GetProactivityRecommendations().begin(), Proto_.GetProactivityRecommendations().end()};
    }

    ::google::protobuf::Map<TString, NScenarios::TFrameAction> GetActions() const override {
        return Proto_.GetActions();
    }

    const TSessionProto::TScenarioSession& GetScenarioSession(const TString& name) const override {
        const auto& sessions = Proto_.GetScenarioSessions();
        if (const auto& it = sessions.find(name); it != sessions.end()) {
            return it->second;
        }
        return TSessionProto::TScenarioSession::default_instance();
    }

    const ::google::protobuf::Map<TString, TSessionProto::TScenarioSession>& GetScenarioSessions() const override {
        return Proto_.GetScenarioSessions();
    }

    const TSessionProto::TScenarioSession& GetPreviousScenarioSession() const override {
        return GetScenarioSession(GetPreviousScenarioName());
    }

    TMaybe<TSemanticFrame::TSlot> GetRequestedSlot() const override {
        TMaybe<TSemanticFrame> frame = GetResponseFrame();
        if (!frame.Defined()) {
            return Nothing();
        }
        return NAlice::GetRequestedSlot(*frame);
    }

    TMaybe<NAlice::TGcMemoryState> GetGcMemoryState() const override {
        if (Proto_.HasGcMemoryState()) {
            return Proto_.GetGcMemoryState();
        }
        return Nothing();
    }

    const NMegamind::TStackEngineCore& GetStackEngineCore() const override {
        return Proto_.GetStackEngineCore();
    }

    bool GetRequestIsExpected() const override {
        return Proto_.GetRequestIsExpected();
    }

    TMaybe<NScenarios::TInput> GetInput() const override {
        if (!Proto_.HasInput()) {
            return Nothing();
        }
        return Proto_.GetInput();
    }

    THolder<ISessionBuilder> GetUpdater() && override;

    THolder<ISessionBuilder> GetUpdater() const & override;

    THolder<ISessionBuilder> CreateBuilder() const override;

    TString Serialize() const override;

    const TSessionProto& Proto() const override {
        return Proto_;
    }

private:
    TSessionProto Proto_;
};

TString TSession::Serialize() const {
    NJson::TJsonValue session;

    session[FIELD_MEGAMIND] = ProtoToBase64String(Proto_);

    // *NOTE* BASE64 + Zlib compression is needed for consistency with
    // VINS sessions.  Don't touch this code without real need.
    TString serialized;
    {
        TStringOutput output(serialized);
        TZLibCompress zlibOutput(&output);
        NJson::WriteJson(&zlibOutput, &session);
    }
    return Base64Encode(serialized);
}


class TSessionBuilder final : public ISessionBuilder {
public:
    TSessionBuilder() = default;

    TSessionBuilder(const TSessionProto& megamindSession)
        : Proto(megamindSession)
    {
    }

    TSessionBuilder(TSessionProto&& megamindSession)
        : Proto(std::move(megamindSession))
    {
    }

    ISessionBuilder& SetPreviousScenarioName(const TString& scenarioName) override {
        Proto.SetPreviousScenarioName(scenarioName);
        return *this;
    }

    ISessionBuilder& SetPreviousProductScenarioName(const TString& scenarioName) override {
        Proto.SetPreviousProductScenarioName(scenarioName);
        return *this;
    }

    ISessionBuilder& SetScenarioResponseBuilder(const TMaybe<TResponseBuilderProto>& responseBuilder) override {
        if (responseBuilder.Defined()) {
            *Proto.MutableScenarioResponseBuilder() = *responseBuilder;
        } else {
            Proto.ClearScenarioResponseBuilder();
        }

        return *this;
    }

    ISessionBuilder& SetLastWhisperTimeMs(const ui64 whisperTimeMs) override {
        Proto.SetLastWhisperServerTimeMs(whisperTimeMs);
        return *this;
    }

    ISessionBuilder& SetDialogHistory(const TDialogHistory& dialogHistory) override {
        auto& dialogHistoryInfo = *Proto.MutableDialogHistoryInfo();
        dialogHistoryInfo.Clear();
        for (const auto& dialogTurn : dialogHistory.GetDialogTurns()) {
            auto& dialogTurnProto = *dialogHistoryInfo.AddDialogTurns();
            dialogTurnProto.SetRequest(dialogTurn.Request);
            dialogTurnProto.SetRewrittenRequest(dialogTurn.RewrittenRequest);
            dialogTurnProto.SetResponse(dialogTurn.Response);
            dialogTurnProto.SetScenarioName(dialogTurn.ScenarioName);
            dialogTurnProto.SetServerTimeMs(dialogTurn.ServerTimeMs);
            dialogTurnProto.SetClientTimeMs(dialogTurn.ClientTimeMs);
        }

        Proto.ClearDialogHistory();
        Proto.ClearRewrittenRequestHistory();

        return *this;
    }

    ISessionBuilder& SetLayout(const NScenarios::TLayout& layout) override {
        *Proto.MutableLayout() = layout;
        return *this;
    }

    ISessionBuilder& SetResponseFrame(const TMaybe<TSemanticFrame>& frame) override {
        if (frame.Defined()) {
            *Proto.MutableResponseFrame() = *frame;
        } else {
            Proto.ClearResponseFrame();
        }

        return *this;
    }

    ISessionBuilder& SetResponseEntities(const ::google::protobuf::RepeatedPtrField<TClientEntity>& entities) override {
        *Proto.MutableResponseEntities() = entities;
        return *this;
    }

    ISessionBuilder& SetProtocolInfo(const TMaybe<TSessionProto::TProtocolInfo>& protocolInfo) override {
        if (protocolInfo.Defined()) {
            *Proto.MutableProtocolInfo() = *protocolInfo;
        } else {
            Proto.ClearProtocolInfo();
        }

        return *this;
    }

    ISessionBuilder&
    SetMegamindAnalyticsInfo(const TMaybe<NMegamind::TMegamindAnalyticsInfo>& analyticsInfo) override {
        if (analyticsInfo) {
            *Proto.MutableMegamindAnalyticsInfo() = *analyticsInfo;
        } else {
            Proto.ClearMegamindAnalyticsInfo();
        }

        return *this;
    }

    ISessionBuilder& SetQualityStorage(const TMaybe<TQualityStorage>& storage) override {
        if (storage) {
            *Proto.MutableQualityStorage() = *storage;
        } else {
            Proto.ClearQualityStorage();
        }

        return *this;
    }

    ISessionBuilder& SetIntentName(const TString& intentName) override {
        Proto.SetIntentName(intentName);
        return *this;
    }

    ISessionBuilder& SetModifiersStorage(const TMaybe<NMegamind::TModifiersStorage>& modifiers) override {
        if (modifiers) {
            *Proto.MutableModifiersStorage() = *modifiers;
        } else {
            Proto.ClearModifiersStorage();
        }

        return *this;
    }

    ISessionBuilder& SetProactivityRecommendations(const TProactivityAnswer& rec) override {
        *Proto.MutableProactivityRecommendations() = {rec.begin(), rec.end()};
        return *this;
    }

    ISessionBuilder& SetActions(const ::google::protobuf::Map<TString, NScenarios::TFrameAction>& actions) override {
        *Proto.MutableActions() = actions;
        return *this;
    }

    ISessionBuilder& SetGcMemoryState(const TMaybe<NAlice::TGcMemoryState>& gcMemoryState) override {
        if (gcMemoryState) {
            *Proto.MutableGcMemoryState() = *gcMemoryState;
        } else {
            Proto.ClearGcMemoryState();
        }
        return *this;
    }

    ISessionBuilder& SetRequestIsExpected(bool requestIsExpected) override {
        Proto.SetRequestIsExpected(requestIsExpected);
        return *this;
    }

    ISessionBuilder& ModifyScenarioSessions(const TScenarioSessionModifier& modifier) override {
        for (auto& [name, session] : *Proto.MutableScenarioSessions()) {
            modifier(name, session);
        }
        return *this;
    }

    ISessionBuilder& SetScenarioSessions(
        const ::google::protobuf::Map<TString, TSessionProto::TScenarioSession>& scenarioSessions) override
    {
        Proto.ClearScenarioSessions();
        Proto.MutableScenarioSessions()->insert(scenarioSessions.begin(), scenarioSessions.end());
        return *this;
    }

    ISessionBuilder& SetScenarioSession(const TString& scenarioName, const TSessionProto::TScenarioSession& proto) override {
        (*Proto.MutableScenarioSessions())[scenarioName] = proto;
        return *this;
    }

    ISessionBuilder& SetStackEngineCore(const NMegamind::TStackEngineCore& core, bool invalidate) override {
        *Proto.MutableStackEngineCore() = core;
        if (invalidate) {
            Proto.MutableStackEngineCore()->SetIsUpdated(false);
        }
        return *this;
    }

    ISessionBuilder& SetInput(const TMaybe<NScenarios::TInput>& input) override {
        if (input.Defined()) {
            *Proto.MutableInput() = *input;
        }
        return *this;
    }

    THolder<ISessionBuilder> Copy() const override {
        return MakeHolder<TSessionBuilder>(*this);
    }

    THolder<ISession> Build() override {
        return MakeHolder<TSession>(Proto);
    }

private:
    TSessionProto Proto;
};

THolder<ISessionBuilder> TSession::GetUpdater() && {
    return MakeHolder<TSessionBuilder>(std::move(Proto_));
}

THolder<ISessionBuilder> TSession::GetUpdater() const & {
    return MakeHolder<TSessionBuilder>(Proto_);
}

THolder<ISessionBuilder> TSession::CreateBuilder() const {
    return MakeHolder<TSessionBuilder>(TSessionProto{});
}

} // namespace

THolder<ISessionBuilder> MakeSessionBuilder() {
    return MakeHolder<TSessionBuilder>();
}

THolder<ISession> MakeMegamindSession(TSessionProto&& megamindSession) {
    if (megamindSession.HasScenarioState() &&
        !MapFindPtr(megamindSession.GetScenarioSessions(), megamindSession.GetPreviousScenarioName()))
    {
        auto& scenarioSessions = *megamindSession.MutableScenarioSessions();
        auto& previousScenarioSession = scenarioSessions[megamindSession.GetPreviousScenarioName()];
        previousScenarioSession.MutableState()->CopyFrom(megamindSession.GetScenarioState());
    }
    return MakeHolder<TSession>(std::move(megamindSession));
}

THolder<ISession> DeserializeMegamindSessionProto(const NJson::TJsonValue& speechkitSession) {
    TSessionProto megamindSession;
    if (speechkitSession.Has(FIELD_MEGAMIND)) {
        ProtoFromBase64String(speechkitSession[FIELD_MEGAMIND].GetString(), megamindSession);
    } else {
        megamindSession.SetPreviousScenarioName(Default<TString>());
        *megamindSession.MutableScenarioState() = TState();
    }

    return MakeMegamindSession(std::move(megamindSession));
}

THolder<ISession> DeserializeSession(const TString& serialized) {
    const NJson::TJsonValue session = ParseRawJsonSession(serialized);

    return DeserializeMegamindSessionProto(session);
}

NJson::TJsonValue ParseRawJsonSession(const TString& serialized) {
    TString decoded = Base64Decode(serialized);
    TStringInput input(decoded);
    TZLibDecompress zlibInput(&input);

    return NJson::ReadJsonTree(&zlibInput, true /* throwOnError */);
}

TSessionProto::TScenarioSession NewScenarioSession(const TState& state) {
    TSessionProto::TScenarioSession scenarioSession;
    *scenarioSession.MutableState() = state;
    return scenarioSession;
}

} // namespace NAlice
