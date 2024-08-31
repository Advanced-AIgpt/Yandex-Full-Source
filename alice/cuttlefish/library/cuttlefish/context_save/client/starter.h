#pragma once

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

#include <alice/cuttlefish/library/protos/context_save.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>

#include <alice/megamind/protos/speechkit/directives.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <apphost/api/service/cpp/service_context.h>

#include <util/generic/maybe.h>

#include <functional>


namespace NAlice::NCuttlefish::NContextSaveClient {

    inline constexpr TStringBuf SAVE_USER_AUDIO_DIRECTIVE_NAME = "save_user_audio";


    class TContextSaveRequestBuilder {
    public:
        using TSpeechKitDirectiveFilterPredicate = TMaybe<std::function<bool(const NAlice::NSpeechKit::TDirective& directive)>>;
        using TProtobufUniproxyDirectiveFilterPredicate = TMaybe<std::function<bool(const NAlice::NSpeechKit::TProtobufUniproxyDirective& directive)>>;

    public:
        TContextSaveRequestBuilder(
            TSpeechKitDirectiveFilterPredicate speechKitDirectiveFilterPredicate = Nothing(),
            TProtobufUniproxyDirectiveFilterPredicate protobufUniproxyDirectiveFilterPredicate = Nothing()
        );

        bool TryAddDirective(const NAlice::NSpeechKit::TDirective& directive);
        bool TryAddContextSaveDirective(const NAlice::NSpeechKit::TProtobufUniproxyDirective& directive);

        NAliceProtocol::TContextSaveRequest Build() &&;

    private:
        NAliceProtocol::TContextSaveRequest ContextSaveRequest;
        TSpeechKitDirectiveFilterPredicate SpeechKitDirectiveFilterPredicate = Nothing();
        TProtobufUniproxyDirectiveFilterPredicate ProtobufUniproxyDirectiveFilterPredicate = Nothing();
    };


    class TContextSaveStarter {
    private:
        template <typename T>
        class TSimpleMaybe : public TMaybe<T> {
        private:
            using TBase = TMaybe<T>;

        public:
            T* Mutable();
        };

    public:
        void SetRequestId(TString reqId);
        void AddClientExperiment(const TString& flag, const TString& value);
        void SetAppId(TString appId);
        void SetPuid(TString puid);

        uint32_t AddDirectives(
            const NAlice::TSpeechKitResponseProto& mmResponse,
            TContextSaveRequestBuilder::TSpeechKitDirectiveFilterPredicate speechKitDirectiveFilterPredicate = Nothing(),
            TContextSaveRequestBuilder::TProtobufUniproxyDirectiveFilterPredicate protobufUniproxyDirectiveFilterPredicate = Nothing()
        );

        // usable for logging
        const NAliceProtocol::TContextSaveRequest& GetRequestRef() const;

        void Finalize(
            NAppHost::IServiceContext& appHostContext,
            const TStringBuf contextSaveRequestItemType,
            const TStringBuf contextSaveNeedFullIncomingAudioFlag
        ) &&;

    private:
        NAliceProtocol::TContextSaveRequest ContextSaveRequest;
        TSimpleMaybe<NAliceProtocol::TSessionContext> SessionCtx;
        TSimpleMaybe<NAliceProtocol::TRequestContext> RequestCtx;
        bool NeedFullIncomingAudio = false;
    };

}  // namespace NAlice::NCuttlefish::NContextSaveClient
