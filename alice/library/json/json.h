#pragma once

#include <library/cpp/json/json_value.h>

#include <library/cpp/protobuf/json/json2proto.h>
#include <library/cpp/protobuf/json/proto2json.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

#include <google/protobuf/util/json_util.h>

namespace NAlice {

inline const auto PROTO2JSON_CONFIG_WITH_JSON_NAMES = NProtobufJson::TProto2JsonConfig{}.SetUseJsonName(true);
inline const auto JSON2PROTO_CONFIG_WITH_JSON_NAMES = NProtobufJson::TJson2ProtoConfig{}.SetUseJsonName(true);

google::protobuf::util::Status JsonToProto(const NJson::TJsonValue& value, google::protobuf::Message& message,
                                           bool validateUtf8 = true, bool ignoreUnknownFields = false);

template <typename TProtoMessage>
TProtoMessage JsonToProto(const NJson::TJsonValue& value, bool validateUtf8 = true, bool ignoreUnknownFields = false) {
    TProtoMessage message;
    const auto status = JsonToProto(value, message, validateUtf8, ignoreUnknownFields);
    Y_ENSURE(status.ok(), "Failed to convert provided Json to Proto: " << status.ToString());
    return message;
}

TString JsonStringFromProto(const google::protobuf::Message& message,
                            const google::protobuf::util::JsonOptions& options = {});

NJson::TJsonValue JsonFromProto(const google::protobuf::Message& message,
                                const google::protobuf::util::JsonOptions& options = {});

TString JsonToString(const NJson::TJsonValue& json, bool validateUtf8 = true, bool formatOutput = false);

NJson::TJsonValue JsonFromString(TStringBuf s);

} // namespace NAlice
