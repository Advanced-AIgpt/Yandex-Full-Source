#pragma once

#include <alice/cuttlefish/library/music_match/base/protobuf.h>
#include <alice/cuttlefish/library/protos/session.pb.h>

#include <voicetech/library/messages/message.h>

namespace NAlice::NCuttlefish::NAppHostServices {
    bool IsMusicMatchRequestViaAsr(
        const NVoicetech::NUniproxy2::TMessage&
    );
    bool IsRecognizeMusicOnly(
        const NVoicetech::NUniproxy2::TMessage&
    );

    void VinsMessageToMusicMatchInitRequest(
        const NVoicetech::NUniproxy2::TMessage&,
        NAlice::NMusicMatch::NProtobuf::TInitRequest&
    );
    void AsrMessageToMusicMatchInitRequest(
        const NVoicetech::NUniproxy2::TMessage&,
        NAlice::NMusicMatch::NProtobuf::TInitRequest&
    );
}
