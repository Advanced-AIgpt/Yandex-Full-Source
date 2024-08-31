#include "servant.h"

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

#include <alice/cuttlefish/library/apphost/item_parser.h>

#include <alice/cuttlefish/library/protos/audio_separator.pb.h>

#include <util/string/cast.h>

namespace NAlice::NCuttlefish::NAppHostServices {

TAudioSeparator::TAudioSeparator(
    NAppHost::TServiceContextPtr ctx,
    TLogContext logContext
)
    : TStreamServantBase(
        ctx,
        logContext,
        TAudioSeparator::SOURCE_NAME
    )
    , State_(EState::WAIT_FOR_BEGIN_STREAM)
{}

bool TAudioSeparator::ProcessFirstChunk() {
    const auto requestContextItemRefs = AhContext_->GetProtobufItemRefs(ITEM_TYPE_REQUEST_CONTEXT, NAppHost::EContextItemSelection::Input);
    if (!requestContextItemRefs.empty()) {
        try {
            ParseProtobufItem(*requestContextItemRefs.begin(), RequestContext_);
        } catch (...) {
            Metrics_.SetError("badrequestcontext");
            OnError(TStringBuilder() << "Failed to parse request context: " << CurrentExceptionMessage(), /* isCritical = */ true);
            return false;
        }
    } else {
        Metrics_.SetError("norequestcontext");
        OnError("Request context not found in first chunk", /* isCritical = */ true);
        return false;
    }

    LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostRequestContext>(RequestContext_.ShortUtf8DebugString());

    return true;
}

bool TAudioSeparator::ProcessInput() {
    try {
        if (!ProcessAudio()) {
            return false;
        }
        if (!ProcessAsrFinished()) {
            return false;
        }
    } catch (...) {
        Metrics_.SetError("badchunk");
        OnError(TStringBuilder() << "Failed to process new apphost chunk: " << CurrentExceptionMessage(), /* isCritical = */ true);
        return false;
    }

    return true;
}

bool TAudioSeparator::IsCompleted() {
    return State_ == EState::FINISHED;
}

TString TAudioSeparator::GetErrorForIncompleteInput() {
    return TStringBuilder() << "Input is incomplete, current state is '" << ToString(State_) << "'";
}

void TAudioSeparator::OnError(const TString& error, bool isCritical) {
    if (isCritical) {
        SendErrorMessage(error);
    }
    TStreamServantBase::OnError(error, isCritical);
}

bool TAudioSeparator::ProcessAudio() {
    const auto audioItemRefs = AhContext_->GetProtobufItemRefs(ITEM_TYPE_AUDIO, NAppHost::EContextItemSelection::Input);

    for (const auto& audioItemRef : audioItemRefs) {
        try {
            NAliceProtocol::TAudio audio;
            ParseProtobufItem(audioItemRef, audio);

            Metrics_.PushRate("audio", "ok");
            if (audio.HasChunk()) {
                NAliceProtocol::TAudio audioExceptChunk = audio;
                audioExceptChunk.ClearChunk();

                Metrics_.PushRate(audio.GetChunk().GetData().size(), "received_total", "ok");
                LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostAudioChunk>(audio.GetChunk().GetData().size(), audioExceptChunk.ShortUtf8DebugString());
            } else {
                LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostAudio>(audio.ShortUtf8DebugString());
            }

            OnAudio(audio);
        } catch (...) {
            Metrics_.SetError("badaudio");
            OnError(TStringBuilder() << "Failed to process audio item: " << CurrentExceptionMessage(), /* isCritical = */ true);
            return false;
        }
    }

    return true;
}

bool TAudioSeparator::ProcessAsrFinished() {
    const auto asrFinishedItemRefs = AhContext_->GetProtobufItemRefs(ITEM_TYPE_ASR_FINISHED, NAppHost::EContextItemSelection::Input);

    if (!asrFinishedItemRefs.empty()) {
        if (asrFinishedItemRefs.size() >= 2u) {
            LogContext_.LogEventInfoCombo<NEvClass::WarningMessage>(TStringBuilder() << "Got " << asrFinishedItemRefs.size() << " asr finished messages");
        }

        NAliceProtocol::TAsrFinished asrFinished;
        try {
            ParseProtobufItem(*asrFinishedItemRefs.begin(), asrFinished);
            LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostAsrFinished>(asrFinished.ShortUtf8DebugString());
            OnAsrFinished();
        } catch (...) {
            Metrics_.SetError("badasrfinished");
            OnError(TStringBuilder() << "Failed to process asr finished: " << CurrentExceptionMessage(), /* isCritical = */ true);
            return false;
        }
    }

    return true;
}

bool TAudioSeparator::OnAudio(const NAliceProtocol::TAudio& audio) {
    switch (audio.GetMessageCase()) {
        case NAliceProtocol::TAudio::MessageCase::kBeginStream: {
            OnBeginStream();
            break;
        }
        case NAliceProtocol::TAudio::MessageCase::kChunk: {
            OnAudioChunk(audio.GetChunk());
            break;
        }
        case NAliceProtocol::TAudio::MessageCase::kEndStream: {
            OnEndStream();
            break;
        }
        case NAliceProtocol::TAudio::MessageCase::kMetaInfoOnly: {
            OnMetaInfoOnly();
            break;
        }
        case NAliceProtocol::TAudio::MessageCase::kBeginSpotter: {
            OnBeginSpotter();
            break;
        }
        case NAliceProtocol::TAudio::MessageCase::kEndSpotter: {
            OnEndSpotter();
            break;
        }
        case NAliceProtocol::TAudio::MessageCase::MESSAGE_NOT_SET: {
            Metrics_.PushRate("audiomessagenotset", "error", "user");
            LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>("Audio message case not set");
            break;
        }
    }

    return true;
}

void TAudioSeparator::OnBeginStream() {
    switch (State_) {
        case EState::WAIT_FOR_BEGIN_STREAM: {
            State_ = EState::WAIT_FOR_MAIN_AUDIO_CHUNK_OR_BEGIN_SPOTTER;
            break;
        }
        case EState::WAIT_FOR_MAIN_AUDIO_CHUNK_OR_BEGIN_SPOTTER:
        case EState::PROCESS_SPOTTER_AUDIO:
        case EState::PROCESS_MAIN_AUDIO:
        case EState::FINISHED: {
            Metrics_.PushRate("unexpectedbeginstream", "error");
            LogContext_.LogEventErrorCombo<NEvClass::WarningMessage>(TStringBuilder() << "BeingStream in '" << ToString(State_) << "' state");
            break;
        }
    }
}

void TAudioSeparator::OnAudioChunk(const NAliceProtocol::TAudioChunk& audioChunk) {
    switch (State_) {
        case EState::PROCESS_SPOTTER_AUDIO: {
            const auto& data = audioChunk.GetData();
            SpotterAudioBuffer_.Append(data.data(), data.size());
            break;
        }
        case EState::WAIT_FOR_MAIN_AUDIO_CHUNK_OR_BEGIN_SPOTTER: {
            State_ = EState::PROCESS_MAIN_AUDIO;
            [[fallthrough]];
        }
        case EState::PROCESS_MAIN_AUDIO: {
            const auto& data = audioChunk.GetData();
            MainAudioBuffer_.Append(data.data(), data.size());
            break;
        }
        case EState::WAIT_FOR_BEGIN_STREAM:
        case EState::FINISHED: {
            Metrics_.PushRate("unexpectedaudiochunk", "error");
            LogContext_.LogEventErrorCombo<NEvClass::WarningMessage>(TStringBuilder() << "AudioChunk in '" << ToString(State_) << "' state");
            break;
        }
    }
}

void TAudioSeparator::OnEndStream() {
    switch (State_) {
        case EState::PROCESS_SPOTTER_AUDIO:
        case EState::PROCESS_MAIN_AUDIO:
        case EState::WAIT_FOR_MAIN_AUDIO_CHUNK_OR_BEGIN_SPOTTER: {
            SendFullIncomingAudio();
            State_ = EState::FINISHED;
            break;
        }
        case EState::WAIT_FOR_BEGIN_STREAM:
        case EState::FINISHED: {
            Metrics_.PushRate("unexpectedendstream", "error");
            LogContext_.LogEventErrorCombo<NEvClass::WarningMessage>(TStringBuilder() << "EndStream in '" << ToString(State_) << "' state");
            break;
        }
    }
}

void TAudioSeparator::OnMetaInfoOnly() {
    // Nothing to do
}

void TAudioSeparator::OnBeginSpotter() {
    switch (State_) {
        case EState::WAIT_FOR_MAIN_AUDIO_CHUNK_OR_BEGIN_SPOTTER: {
            State_ = EState::PROCESS_SPOTTER_AUDIO;
            break;
        }
        case EState::WAIT_FOR_BEGIN_STREAM:
        case EState::PROCESS_SPOTTER_AUDIO:
        case EState::PROCESS_MAIN_AUDIO:
        case EState::FINISHED: {
            Metrics_.PushRate("unexpectedbeginspotter", "error");
            LogContext_.LogEventErrorCombo<NEvClass::WarningMessage>(TStringBuilder() << "BeginSpotter in '" << ToString(State_) << "' state");
            break;
        }
    }
}

void TAudioSeparator::OnEndSpotter() {
    switch (State_) {
        case EState::PROCESS_SPOTTER_AUDIO: {
            State_ = EState::PROCESS_MAIN_AUDIO;
            break;
        }
        case EState::WAIT_FOR_BEGIN_STREAM:
        case EState::WAIT_FOR_MAIN_AUDIO_CHUNK_OR_BEGIN_SPOTTER:
        case EState::PROCESS_MAIN_AUDIO:
        case EState::FINISHED: {
            Metrics_.PushRate("unexpectedendspotter", "error");
            LogContext_.LogEventErrorCombo<NEvClass::WarningMessage>(TStringBuilder() << "EndSpotter in '" << ToString(State_) << "' state");
            break;
        }
    }
}

void TAudioSeparator::OnAsrFinished() {
    SendFullIncomingAudio();
    State_ = EState::FINISHED;
}

void TAudioSeparator::SendFullIncomingAudio() {
    NAliceProtocol::TFullIncomingAudio fullIncomingAudio;

    if (!SpotterAudioBuffer_.Empty()) {
        Metrics_.PushRate(SpotterAudioBuffer_.size(), "received_spotter", "ok");
        fullIncomingAudio.SetSpotterPart(TString(SpotterAudioBuffer_.Begin(), SpotterAudioBuffer_.End()));
    }
    if (!MainAudioBuffer_.Empty()) {
        Metrics_.PushRate(MainAudioBuffer_.size(), "received_main", "ok");
        fullIncomingAudio.SetMainPart(TString(MainAudioBuffer_.Begin(), MainAudioBuffer_.End()));
    }

    LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostFullIncomingAudio>(
        fullIncomingAudio.GetSpotterPart().size(),
        fullIncomingAudio.GetMainPart().size(),
        "" // Other fields are empty
    );

    AhContext_->AddProtobufItem(fullIncomingAudio, ITEM_TYPE_FULL_INCOMING_AUDIO);
    AhContext_->IntermediateFlush();
}

void TAudioSeparator::SendErrorMessage(const TString& error) {
    NAliceProtocol::TFullIncomingAudio fullIncomingAudio;

    fullIncomingAudio.SetErrorMessage(error);
    LogContext_.LogEventErrorCombo<NEvClass::SendToAppHostFullIncomingAudio>(0, 0, fullIncomingAudio.ShortUtf8DebugString());

    AhContext_->AddProtobufItem(fullIncomingAudio, ITEM_TYPE_FULL_INCOMING_AUDIO);
    AhContext_->IntermediateFlush();
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
