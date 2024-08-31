#pragma once

#include <alice/cuttlefish/library/music_match/base/protobuf.h>
#include <alice/cuttlefish/library/protos/session.pb.h>

#include <voicetech/library/messages/message.h>

namespace NAlice::NCuttlefish::NAppHostServices {
    void MusicMatchStreamResponseToJson(const NAlice::NMusicMatch::NProtobuf::TStreamResponse&, NJson::TJsonValue& payload);
}
