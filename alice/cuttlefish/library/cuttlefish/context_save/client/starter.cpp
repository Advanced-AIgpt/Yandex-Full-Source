#include <alice/cuttlefish/library/cuttlefish/context_save/client/starter.h>

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>


namespace NAlice::NCuttlefish::NContextSaveClient {

    TContextSaveRequestBuilder::TContextSaveRequestBuilder(
        TSpeechKitDirectiveFilterPredicate speechKitDirectiveFilterPredicate,
        TProtobufUniproxyDirectiveFilterPredicate protobufUniproxyDirectiveFilterPredicate
    )
        : SpeechKitDirectiveFilterPredicate(std::move(speechKitDirectiveFilterPredicate))
        , ProtobufUniproxyDirectiveFilterPredicate(std::move(protobufUniproxyDirectiveFilterPredicate))
    {
    }

    bool TContextSaveRequestBuilder::TryAddDirective(const NAlice::NSpeechKit::TDirective& directive) {
        if (SpeechKitDirectiveFilterPredicate.Defined() && !(SpeechKitDirectiveFilterPredicate.GetRef()(directive))) {
            return false;
        }

        ContextSaveRequest.AddDirectives()->CopyFrom(directive);
        return true;
    }

    bool TContextSaveRequestBuilder::TryAddContextSaveDirective(
        const NAlice::NSpeechKit::TProtobufUniproxyDirective& directive
    ) {
        if (ProtobufUniproxyDirectiveFilterPredicate.Defined() && !(ProtobufUniproxyDirectiveFilterPredicate.GetRef()(directive))) {
            return false;
        }

        if (!directive.HasContextSaveDirective()) {
            return false;
        }

        ContextSaveRequest.AddContextSaveDirectives()->CopyFrom(directive.GetContextSaveDirective());
        return true;
    }

    NAliceProtocol::TContextSaveRequest TContextSaveRequestBuilder::Build() && {
        return std::move(ContextSaveRequest);
    }

    template <typename T>
    T* TContextSaveStarter::TSimpleMaybe<T>::Mutable() {
        if (!TBase::Defined()) {
            TBase::ConstructInPlace();
        }
        return TBase::Get();
    }

    void TContextSaveStarter::SetRequestId(TString reqId) {
        RequestCtx.Mutable()->MutableHeader()->SetReqId(std::move(reqId));
    }

    void TContextSaveStarter::AddClientExperiment(const TString& flag, const TString& value) {
        RequestCtx.Mutable()->MutableExpFlags()->insert({flag, value});
    }

    void TContextSaveStarter::SetAppId(TString appId) {
        SessionCtx.Mutable()->SetAppId(std::move(appId));
    }

    void TContextSaveStarter::SetPuid(TString puid) {
        SessionCtx.Mutable()->MutableUserInfo()->SetPuid(std::move(puid));
    }

    uint32_t TContextSaveStarter::AddDirectives(
        const NAlice::TSpeechKitResponseProto& mmResponse,
        TContextSaveRequestBuilder::TSpeechKitDirectiveFilterPredicate speechKitDirectiveFilterPredicate,
        TContextSaveRequestBuilder::TProtobufUniproxyDirectiveFilterPredicate protobufUniproxyDirectiveFilterPredicate
    ) {
        uint32_t added = 0;
        const auto& voiceResponse = mmResponse.GetVoiceResponse();
        TContextSaveRequestBuilder builder(
            std::move(speechKitDirectiveFilterPredicate),
            std::move(protobufUniproxyDirectiveFilterPredicate)
        );
        for (const NAlice::NSpeechKit::TDirective& directive : voiceResponse.GetDirectives()) {
            if (builder.TryAddDirective(directive)) {
                if (directive.GetName() == SAVE_USER_AUDIO_DIRECTIVE_NAME) {
                    NeedFullIncomingAudio = true;
                }
                ++added;
            }
        }
        for (const auto& directive : voiceResponse.GetUniproxyDirectives()) {
            if (builder.TryAddContextSaveDirective(directive)) {
                ++added;
            }
        }
        ContextSaveRequest = std::move(builder).Build();
        return added;
    }

    const NAliceProtocol::TContextSaveRequest& TContextSaveStarter::GetRequestRef() const {
        return ContextSaveRequest;
    }

    void TContextSaveStarter::Finalize(
        NAppHost::IServiceContext& appHostContext,
        const TStringBuf contextSaveRequestItemType,
        const TStringBuf contextSaveNeedFullIncomingAudioFlag
    ) && {
        // session_context and request_context are created only in gproxy.
        // uniproxy/voice_input already has this items.
        if (SessionCtx.Defined()) {
            appHostContext.AddProtobufItem(std::move(SessionCtx.GetRef()), ITEM_TYPE_SESSION_CONTEXT);
        }
        if (RequestCtx.Defined()) {
            appHostContext.AddProtobufItem(std::move(RequestCtx.GetRef()), ITEM_TYPE_REQUEST_CONTEXT);
        }

        // Finalize should not be called if AddDirectives returned false.
        Y_ASSERT(!ContextSaveRequest.GetDirectives().empty());
        appHostContext.AddProtobufItem(std::move(ContextSaveRequest), contextSaveRequestItemType);

        if (NeedFullIncomingAudio) {
            appHostContext.AddFlag(contextSaveNeedFullIncomingAudioFlag);
        }
    }

}  // namespace NAlice::NCuttlefish::NContextSaveClient
