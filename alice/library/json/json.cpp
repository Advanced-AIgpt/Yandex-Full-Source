#include "json.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

#include <util/stream/str.h>

namespace NAlice {

google::protobuf::util::Status JsonToProto(const NJson::TJsonValue& value, google::protobuf::Message& message,
                                           bool validateUtf8, bool ignoreUnknownFields) {
    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = ignoreUnknownFields;
    return google::protobuf::util::JsonStringToMessage(JsonToString(value, validateUtf8), &message, options);
}

/*
    Converts Google Protobuf to Json string
    Note this function uses standard protobuf conversion and doesn't work properly with int64 fields.
    Values in int64 will be repesente as 'String' tye in json.
    For more details please check https://github.com/protocolbuffers/protobuf/issues/2679 and
    https://developers.google.com/protocol-buffers/docs/proto3#json

    To avoid these problems you may use proto->json conversion from 
    https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/protobuf/json
    instead of this function.
*/
TString JsonStringFromProto(const google::protobuf::Message& message,
                            const google::protobuf::util::JsonOptions& options) {
    TString buffer;
    const auto status = google::protobuf::util::MessageToJsonString(message, &buffer, options);
    Y_ENSURE(status.ok(), "Failed to convert provided Proto to Json: " << status.ToString());
    return buffer;
}

NJson::TJsonValue JsonFromProto(const google::protobuf::Message& message,
                                const google::protobuf::util::JsonOptions& options) {
    return JsonFromString(JsonStringFromProto(message, options));
}

TString JsonToString(const NJson::TJsonValue& json, const bool validateUtf8, const bool formatOutput) {
    TStringStream s;
    NJson::TJsonWriterConfig config;
    config.SortKeys = true;
    config.ValidateUtf8 = validateUtf8;
    config.FormatOutput = formatOutput;
    NJson::TJsonWriter{&s, config}.Write(&json);
    return s.Str();
}

NJson::TJsonValue JsonFromString(const TStringBuf s) {
    return NJson::ReadJsonFastTree(s, /* notClosedBracketIsError */ true);
}

} // namespace NAlice
