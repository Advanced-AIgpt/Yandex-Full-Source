#pragma once

#include <alice/megamind/library/models/directives/update_datasync_directive_model.h>
#include <alice/megamind/library/models/interfaces/directive_model.h>
#include <alice/megamind/library/request/request.h>
#include <alice/megamind/library/request/event/event.h>
#include <alice/megamind/library/scenarios/interface/response_builder.h>
#include <alice/megamind/library/serializers/speechkit_proto_serializer.h>
#include <alice/megamind/library/session/session.h>
#include <alice/megamind/library/speechkit/request.h>
#include <alice/megamind/library/stack_engine/stack_engine.h>
#include <alice/megamind/library/util/status.h>

#include <alice/megamind/protos/analytics/user_info.pb.h>
#include <alice/megamind/protos/common/directives_execution_policy.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <alice/library/proto/protobuf.h>

#include <google/protobuf/util/message_differencer.h>

#include <util/generic/iterator_range.h>
#include <util/generic/maybe.h>
#include <util/generic/noncopyable.h>
#include <util/generic/string.h>

#include <functional>
#include <memory>
#include <utility>

namespace NAlice {

class TResponseBuilder : public IResponseBuilder, public NNonCopyable::TMoveOnly {
public:
    struct TUserInfoWrapper {
        using TProto = TUserInfo;
    };

public:
    TResponseBuilder(const TSpeechKitRequest& skr, const TRequest& requestModel,
                     const TString& scenarioName, TResponseBuilderProto& storage,
                     const NMegamind::IGuidGenerator& guidGenerator = NMegamind::TGuidGenerator(),
                     const TMaybe<TString>& serializerScenarioName = Nothing());

    /** Reset internal data but keep scenario name.
     */
    void Reset(const TSpeechKitRequest& skr, const TRequest& requestModel,
               const NMegamind::IGuidGenerator& guidGenerator);

    /** Reset internal data with new scenario name.
     */
    void Reset(const TSpeechKitRequest& skr, const TRequest& requestModel,
               const NMegamind::IGuidGenerator& guidGenerator, const TString& scenarioName);

    IResponseBuilder& AddSimpleText(const TString& text, bool appendTts = false) override;
    IResponseBuilder& AddSimpleText(const TString& text, const TString& tts, bool appendTts = false) override;

    IResponseBuilder& AddCard(const NMegamind::ICardModel& card) override;
    IResponseBuilder& AddDirective(const NMegamind::IDirectiveModel& directive) override;
    IResponseBuilder& AddFrontDirective(const NMegamind::IDirectiveModel& directive) override;
    IResponseBuilder& AddDirectiveToVoiceResponse(const NMegamind::IDirectiveModel& directive) override;
    IResponseBuilder& AddSuggest(const NMegamind::IButtonModel& button) override;
    IResponseBuilder& SetOutputSpeech(const TString& text) override;
    IResponseBuilder& SetOutputEmotion(const TString& outputEmotion) override;
    IResponseBuilder& SetResponseErrorMessage(const TResponseErrorMessage& responseErrorMessage) override;

    IResponseBuilder& AddProtobufUniproxyDirective(const NMegamind::IDirectiveModel& model) override;

    // Can be blocked by Storage.ForceDisableShouldListen
    IResponseBuilder& ShouldListen(bool value) override;
    IResponseBuilder& SetIsTrashPartial(bool value) override;
    IResponseBuilder& SetContentProperties(const TContentProperties& contentProperties) override;
    IResponseBuilder& AddError(const TString& errorType, const TString& message) override;

    IResponseBuilder& AddAnalyticsAttention(const TString& type) override;

    IResponseBuilder& AddMeta(const TString& type, const NJson::TJsonValue& payload) override;
    IResponseBuilder& SetSession(const TString& dialogId, const TString& session) {
        (*GetSKRMutableProto().MutableSessions())[dialogId] = session;
        return *this;
    }
    template <typename TTemplates>
    IResponseBuilder& SetDiv2Templates(TTemplates&& templates) {
        *GetSKRMutableProto().MutableResponse()->MutableTemplates() = std::forward<TTemplates>(templates);
        return *this;
    }
    IResponseBuilder& SetStackEngine(std::unique_ptr<NMegamind::IStackEngine> stackEngine) {
        StackEngine = std::move(stackEngine);
        return *this;
    }
    IResponseBuilder& SetVersion(const TString& version) {
        Storage().MutableResponse()->SetVersion(version);
        return *this;
    }
    IResponseBuilder& SetLayout(std::unique_ptr<NScenarios::TLayout> layout) {
        Layout = std::move(layout);
        return *this;
    }

    IResponseBuilder& SetProductScenarioName(const TString& psn) override;

    const TContentProperties& GetContentProperties() const {
        return Storage().GetResponse().GetContentProperties();
    }

    const NMegamind::IStackEngine* GetStackEngine() const {
        return StackEngine.get();
    }

    const NScenarios::TLayout* GetLayout() const {
        return Layout.get();
    }

    IResponseBuilder& SetDirectivesExecutionPolicy(EDirectivesExecutionPolicy value) override;

    const TResponseBuilderProto& ToProto() const {
        return ProtoStorage;
    }

    static TResponseBuilder FromProto(const TSpeechKitRequest& skr, const TRequest& requestModel,
                                      TResponseBuilderProto& proto,
                                      const NMegamind::IGuidGenerator& guidGenerator);

    const TSpeechKitResponseProto& GetSKRProto() const;
    TSpeechKitResponseProto& GetSKRMutableProto();

    TString GetRenderedSpeech() const;
    TString GetRenderedText() const;
    TString GetRenderedResponse() const;
    bool GetShouldListen(bool def) const;

    TStatus PutSemanticFrame(const TSemanticFrame& frame);
    TMaybe<TSemanticFrame> GetSemanticFrame() const;
    TStatus PutActions(const ::google::protobuf::Map<TString, NScenarios::TFrameAction>& actions);
    ::google::protobuf::Map<TString, NScenarios::TFrameAction> GetActions() const;
    IResponseBuilder& PutAction(const TString& key, const NScenarios::TFrameAction& action);
    TStatus PutEntities(const ::google::protobuf::RepeatedPtrField<TClientEntity>& entities);
    ::google::protobuf::RepeatedPtrField<TClientEntity> GetEntities() const;

    const NMegamind::TSerializerMeta& GetSerializerMeta() const {
        return SerializerMeta;
    }

    bool operator==(const TResponseBuilder& rhs) const {
        return google::protobuf::util::MessageDifferencer::Equivalent(GetSKRProto(), rhs.GetSKRProto());
    }

    IResponseBuilder& SetStackEngineParentRequestId(const TString& requestId) {
        Storage().MutableResponse()->MutableHeader()->SetParentRequestId(requestId);
        return *this;
    }

    const TString& GetProductScenarioName() const {
        return Storage().GetProductScenarioName();
    }

private:
    // Init response builder from initialized protobuf.
    TResponseBuilder(const TSpeechKitRequest& skr, const TRequest& requestModel,
                     TResponseBuilderProto& proto,
                     const NMegamind::IGuidGenerator& guidGenerator);

    // Init empty proto.
    void InitProtoFromScratch(const TSpeechKitRequest& request, const TRequest& requestModel,
                              const TString& scenarioName,
                              const NMegamind::IGuidGenerator& guidGenerator,
                              const TMaybe<TString>& serializerScenarioName = Nothing());
    // Init additional required field in already initialized proto.
    void InitRequiredFields(const TSpeechKitRequest& request, const TRequest& requestModel,
                            const NMegamind::IGuidGenerator& guidGenerator,
                            const TMaybe<TString>& serializerScenarioName = Nothing());

    const TResponseBuilderProto& Storage() const {
        return ProtoStorage;
    }

    TResponseBuilderProto& Storage() {
        return ProtoStorage;
    }

private:
    std::reference_wrapper<TResponseBuilderProto> ProtoStorage;
    NMegamind::TSerializerMeta SerializerMeta;
    NMegamind::TSpeechKitProtoSerializer SpeechKitProtoSerializer;
    std::unique_ptr<NMegamind::IStackEngine> StackEngine;
    std::unique_ptr<NScenarios::TLayout> Layout;
};

} // namespace NAlice
