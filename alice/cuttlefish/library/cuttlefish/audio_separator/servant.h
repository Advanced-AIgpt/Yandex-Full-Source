#pragma once

#include <alice/cuttlefish/library/cuttlefish/stream_servant_base/base.h>

#include <alice/cuttlefish/library/protos/asr.pb.h>
#include <alice/cuttlefish/library/protos/audio.pb.h>

namespace NAlice::NCuttlefish::NAppHostServices {

// Parse input user audio stream (spotter and main part) and collect them in one protobuf message
// It is needed to prevent copypaste of the parse logic in other servants (in store_audio and context_save graphs for example)
class TAudioSeparator : public TStreamServantBase {
public:
    // Public for GENERATE_ENUM_SERIALIZATION
    enum EState {
        WAIT_FOR_BEGIN_STREAM = 0                         /* "wait_for_begin_stream" */,
        WAIT_FOR_MAIN_AUDIO_CHUNK_OR_BEGIN_SPOTTER = 1    /* "wait_for_main_audio_chunk_or_begin_spotter" */,
        PROCESS_SPOTTER_AUDIO = 2                         /* "process_spotter_audio" */,
        PROCESS_MAIN_AUDIO = 3                            /* "process_main_audio" */,
        FINISHED = 4                                      /* "finished" */,
    };

    /*
        WAIT_FOR_BEGIN_STREAM
                 |                                                         (AudioChunk)
                 | (BeginStream)                                          +----------->+
                 |                                                        ^            |
                 V                                    (BeginSpotter)      |            V
        WAIT_FOR_MAIN_AUDIO_CHUNK_OR_BEGIN_SPOTTER ------------------> PROCESS_SPOTTER_AUDIO
        |                            |                                        |            |
        |                +-->+       | (AudioChunk)              (EndSpotter) |            |
        | (EndStream)    ^   |       |                                        |            |
        |                |   V       V                                        V            |
        |   (AudioChunk) |   PROCESS_MAIN_AUDIO <-----------------------------+            | (EndStream)
        |                |   |       |                                                     |
        |                |   V       | (EndStream)                                         |
        |                +<--+       |                                                     |
        V                            V                                                     V
        +--------------------------->+---------------------> FINISHED <--------------------+

        (AsrFinished) ---> FINISHED from all states
    */

public:
    TAudioSeparator(
        NAppHost::TServiceContextPtr ctx,
        TLogContext logContext
    );

protected:
    bool ProcessFirstChunk() override;
    bool ProcessInput() override;
    bool IsCompleted() override;
    TString GetErrorForIncompleteInput() override;
    void OnError(const TString& error, bool isCritical) override;

private:
    bool ProcessAudio();
    bool ProcessAsrFinished();

    bool OnAudio(const NAliceProtocol::TAudio& audio);
    void OnBeginStream();
    void OnAudioChunk(const NAliceProtocol::TAudioChunk& audioChunk);
    void OnEndStream();
    void OnMetaInfoOnly();
    void OnBeginSpotter();
    void OnEndSpotter();

    void OnAsrFinished();

    void SendFullIncomingAudio();
    void SendErrorMessage(const TString& error);

private:
    static constexpr TStringBuf SOURCE_NAME = "audio_separator";

    NAliceProtocol::TRequestContext RequestContext_;
    EState State_;

    TBuffer SpotterAudioBuffer_;
    TBuffer MainAudioBuffer_;
};

}  // namespace NAlice::NCuttlefish::NAppHostServices
