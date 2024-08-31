#pragma once

#include <alice/cuttlefish/library/asr/base/protobuf.h>
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <voicetech/library/messages/message.h>

namespace NAlice::NCuttlefish::NAppHostServices {
    void AsrResponseToJson(const NAlice::NAsr::NProtobuf::TResponse&, NJson::TJsonValue& payload);
    bool HasEmptyText(const NAlice::NAsr::NProtobuf::TResponse&);
}
