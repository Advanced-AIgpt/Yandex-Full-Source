#pragma once

#include <voicetech/library/proto_api/yaldi.pb.h>

namespace NAlice::NAsrAdapter {
    YaldiProtobuf::InitRequest GetCensoredYaldiInitRequest(const YaldiProtobuf::InitRequest& asrInitRequest);
}
