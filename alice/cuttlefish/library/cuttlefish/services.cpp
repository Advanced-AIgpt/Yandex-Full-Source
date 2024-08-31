#include "services.h"

#include <alice/cuttlefish/library/api/handlers.h>

#include <alice/cuttlefish/library/cuttlefish/any_input/service.h>
#include <alice/cuttlefish/library/cuttlefish/audio_separator/service.h>
#include <alice/cuttlefish/library/cuttlefish/bio_context_load/service.h>
#include <alice/cuttlefish/library/cuttlefish/bio_context_save/service.h>
#include <alice/cuttlefish/library/cuttlefish/bio_context_sync/service.h>
#include <alice/cuttlefish/library/cuttlefish/context_load/service.h>
#include <alice/cuttlefish/library/cuttlefish/context_save/service.h>
#include <alice/cuttlefish/library/cuttlefish/converter/service.h>
#include <alice/cuttlefish/library/cuttlefish/fake_synchronize_state/service.h>
#include <alice/cuttlefish/library/cuttlefish/guest_context_load/service.h>
#include <alice/cuttlefish/library/cuttlefish/log_spotter/service.h>
#include <alice/cuttlefish/library/cuttlefish/megamind/service.h>
#include <alice/cuttlefish/library/cuttlefish/session_logs_collector/service.h>
#include <alice/cuttlefish/library/cuttlefish/store_audio/service.h>
#include <alice/cuttlefish/library/cuttlefish/stream_converter/service.h>
#include <alice/cuttlefish/library/cuttlefish/synchronize_state/service.h>
#include <alice/cuttlefish/library/cuttlefish/tts/aggregator/service/service.h>
#include <alice/cuttlefish/library/cuttlefish/tts/request_sender/service.h>
#include <alice/cuttlefish/library/cuttlefish/tts/splitter/service.h>

using namespace NAlice::NCuttlefish;

namespace {
    template <void(*ServiceFunc)(const NAliceCuttlefishConfig::TConfig&, NAppHost::IServiceContext&, TLogContext)>
    auto WithConfiguration(const NAliceCuttlefishConfig::TConfig& config) {
        return [&config](NAppHost::IServiceContext& ctx, TLogContext logContext) {
            ServiceFunc(config, ctx, std::move(logContext));
        };
    }

    template <NThreading::TPromise<void>(*ServiceFunc)(const NAliceCuttlefishConfig::TConfig&, NAppHost::TServiceContextPtr, TLogContext)>
    auto WithConfiguration(const NAliceCuttlefishConfig::TConfig& config) {
        return [&config](NAppHost::TServiceContextPtr ctx, TLogContext logContext) {
            return ServiceFunc(config, ctx, std::move(logContext));
        };
    }
}  // anonymous namespace

TVector<TServiceHandler> NAlice::NCuttlefish::GetAppHostServices(const NAliceCuttlefishConfig::TConfig& config) {
    using namespace NAlice::NCuttlefish::NAppHostServices;

    return TVector<TServiceHandler>({
        {SERVICE_HANDLE_ANY_INPUT_PRE, &AnyInputPre},
        {SERVICE_HANDLE_AUDIO_SEPARATOR, &AudioSeparator},
        {SERVICE_HANDLE_BIO_CONTEXT_LOAD_POST, &BioContextLoadPost},
        {SERVICE_HANDLE_BIO_CONTEXT_SAVE_POST, WithConfiguration<BioContextSavePost>(config)},
        {SERVICE_HANDLE_BIO_CONTEXT_SAVE_PRE, WithConfiguration<BioContextSavePre>(config)},
        {SERVICE_HANDLE_BIO_CONTEXT_SYNC, &BioContextSync},
        {SERVICE_HANDLE_CONTEXT_LOAD_BLACKBOX_SETDOWN, &ContextLoadBlackboxSetdown},
        {SERVICE_HANDLE_CONTEXT_LOAD_MAKE_CONTACTS_REQUEST, &ContextLoadMakeContactsRequest},
        {SERVICE_HANDLE_CONTEXT_LOAD_POST, &ContextLoadPost},
        {SERVICE_HANDLE_CONTEXT_LOAD_PRE, &ContextLoadPre},
        {SERVICE_HANDLE_CONTEXT_LOAD_PREPARE_FLAGS_JSON, &ContextLoadPrepareFlagsJson},
        {SERVICE_HANDLE_CONTEXT_LOAD_PREPARE_LAAS, &ContextLoadPrepareLaas},
        {SERVICE_HANDLE_CONTEXT_SAVE_POST, &ContextSavePost},
        {SERVICE_HANDLE_CONTEXT_SAVE_PRE, &ContextSavePre},
        {SERVICE_HANDLE_FAKE_CONTEXT_LOAD, &FakeContextLoad},
        {SERVICE_HANDLE_FAKE_CONTEXT_SAVE, &FakeContextSave},
        {SERVICE_HANDLE_FAKE_SYNCHRONIZE_STATE, WithConfiguration<FakeSynchronizeState>(config)},
        {SERVICE_HANDLE_GUEST_CONTEXT_LOAD_BLACKBOX_SETDOWN, &GuestContextLoadBlackboxSetdown},
        {SERVICE_HANDLE_LOG_SPOTTER, &LogSpotter},
        {SERVICE_HANDLE_MEGAMIND_APPLY, WithConfiguration<MegamindApply>(config)},
        {SERVICE_HANDLE_MEGAMIND_RUN, WithConfiguration<MegamindRun>(config)},
        {SERVICE_HANDLE_PROTOBUF_TO_RAW, &ProtobufToRaw},
        {SERVICE_HANDLE_RAW_TO_PROTOBUF, &RawToProtobuf},
        {SERVICE_HANDLE_SESSION_LOGS_COLLECTOR, &SessionLogsCollector},
        {SERVICE_HANDLE_STORE_AUDIO_POST, StoreAudioPost},
        {SERVICE_HANDLE_STORE_AUDIO_PRE, WithConfiguration<StoreAudioPre>(config)},
        {SERVICE_HANDLE_STREAM_PROTOBUF_TO_RAW, &StreamProtobufToRaw},
        {SERVICE_HANDLE_STREAM_RAW_TO_PROTOBUF, &StreamRawToProtobuf},
        {SERVICE_HANDLE_SYNCHRONIZE_STATE_BLACKBOX_SETDOWN, WithConfiguration<SynchronizeStateBlackboxSetdown>(config)},
        {SERVICE_HANDLE_SYNCHRONIZE_STATE_POST, WithConfiguration<SynchronizeStatePostprocess>(config)},
        {SERVICE_HANDLE_SYNCHRONIZE_STATE_PRE, WithConfiguration<SynchronizeStatePreprocess>(config)},
        {SERVICE_HANDLE_TTS_AGGREGATOR, &TtsAggregator},
        {SERVICE_HANDLE_TTS_REQUEST_SENDER, &TtsRequestSender},
        {SERVICE_HANDLE_TTS_SPLITTER, WithConfiguration<TtsSplitter>(config)},
    });
}
