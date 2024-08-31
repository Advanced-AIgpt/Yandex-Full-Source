#pragma once
#include <alice/cuttlefish/library/speechkit_proto/client_ws.pb.h>

#include <util/generic/maybe.h>
#include <json/json.h>

namespace NSpeechkitProtocol {
    const int PROTOCOL_VERSION = 1;
    TMaybe<Json::Value> DecodeMessage(const TSpeechkitMessage &);
    TMaybe<TSpeechkitMessage> EncodeMessage(const Json::Value &);
}
