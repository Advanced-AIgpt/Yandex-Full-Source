#include "megamind.h"

#include <alice/wonderlogs/library/parsers/internal/utils.h>

#include <alice/wonderlogs/library/common/names.h>
#include <alice/wonderlogs/library/common/utils.h>

#include <alice/library/json/json.h>
#include <alice/megamind/protos/analytics/megamind_analytics_info.pb.h>
#include <alice/megamind/protos/common/required_messages/required_messages.pb.h>
#include <alice/megamind/protos/quality_storage/storage.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/yson/json2yson.h>

#include <util/string/builder.h>

namespace {

const TString DEFER_APPLY_DIRECTIVE = "defer_apply";
constexpr TStringBuf RESPONSE_KEY = "response";
constexpr TStringBuf HEADER_KEY = "header";

} // namespace

namespace NAlice::NWonderlogs {

TMegamindLogsParser::TMegamindLogsParser(const NYT::TNode& row)
    : Row(row) {
}

TMegamindParsedLogs TMegamindLogsParser::Parse() const {
    // all possible google.protobuf.Any values should be linked inside MM while jsonToProto conversion exists
    const NAlice::NMegamind::TRequiredMessages requiredProtoMessages;
    Y_UNUSED(requiredProtoMessages);
    TMegamindParsedLogs res;
    if (Row["content_type"].AsString() != "application/json") {
        return res;
    }

    TMegamindPrepared::TMegamindRequestResponse requestResponse;

    const auto uuid = ParseUuid(Row);
    const auto requestId = ParseRequestId(Row);
    const auto responseId = ParseResponseId(Row);
    const auto messageId = ParseMessageId(Row);

    if (uuid) {
        requestResponse.SetUuid(*uuid);
    }
    if (requestId) {
        requestResponse.SetRequestId(*requestId);
    }
    if (messageId) {
        requestResponse.SetMessageId(*messageId);
    }
    if (responseId) {
        requestResponse.SetResponseId(*responseId);
    }

    auto& errors = res.Errors;

    const auto convertYson2Proto = [&errors, &uuid, &requestId,
                                    &messageId](const NYT::TNode& node, auto& proto,
                                                const std::function<void(NJson::TJsonValue&)>* jsonPatcher) {
        NJson::TJsonValue json;
        const auto jsonString = NJson2Yson::ConvertYson2Json(NYT::NodeToYsonString(node));
        if (!NJson::ReadJsonTree(jsonString, &json)) {
            errors.push_back(GenerateError(TMegamindPrepared::TError::R_FAILED_CONVERT_YSON_TO_JSON,
                                           NYT::NodeToYsonString(node), uuid, requestId, messageId));
            return false;
        }
        if (jsonPatcher) {
            (*jsonPatcher)(json);
        }

        if (const auto status =
                NAlice::JsonToProto(json, proto, /* validateUtf8= */ true, /* ignoreUnknownFields= */ true);
            !status.ok()) {
            errors.push_back(GenerateError(TMegamindPrepared::TError::R_FAILED_CONVERT_JSON_TO_PROTO,
                                           (TStringBuilder{} << status.ToString() << " " << ToString(json)), uuid,
                                           requestId, messageId));
            return false;
        }
        return true;
    };
    {
        bool skipRow = false;
        // TODO add std::tie(messageId, MESSAGE_ID, row["response"])
        for (const auto& [field, name, yson] :
             {std::tie(uuid, UUID, Row["uuid"]), std::tie(requestId, REQUEST_ID, Row["request_id"]),
              std::tie(responseId, RESPONSE_ID, Row["response"])}) {
            if (!NotEmpty(field)) {
                skipRow = true;
                errors.push_back(
                    GenerateError(TMegamindPrepared::TError::R_INVALID_VALUE,
                                  TStringBuilder{} << "Invalid " << name << ": " << NYT::NodeToYsonString(yson), uuid,
                                  requestId, messageId));
            }
        }
        if (skipRow) {
            return res;
        }
    }

    requestResponse.MutableEnvironment()->SetEnvironment(Row["environment"].AsString());
    requestResponse.MutableEnvironment()->SetProvider(Row["provider"].AsString());

    if (!convertYson2Proto(Row["request"], *requestResponse.MutableSpeechKitRequest(), nullptr)) {
        return res;
    }

    {
        const std::function<void(NJson::TJsonValue&)> jsonPatcher = [](NJson::TJsonValue& responseJson) {
            if (auto* metas = responseJson.GetValueByPath("response.meta"); metas && metas->IsArray()) {
                for (auto& meta : metas->GetArraySafe()) {
                    if (auto* slots = meta.GetValueByPath("form.slots"); slots && slots->IsDefined()) {
                        for (auto& slot : slots->GetArraySafe()) {
                            slot = NJson::WriteJson(&slot);
                        }
                    }
                }
            }
        };
        if (!convertYson2Proto(Row["response"], *requestResponse.MutableSpeechKitResponse(), &jsonPatcher)) {
            return res;
        }
    }

    if (!convertYson2Proto(Row["analytics_info"],
                           *requestResponse.MutableSpeechKitResponse()->MutableMegamindAnalyticsInfo(), nullptr)) {
        return res;
    }

    if (!convertYson2Proto(Row["quality_storage"],
                           *requestResponse.MutableSpeechKitResponse()->MutableResponse()->MutableQualityStorage(),
                           nullptr)) {
        return res;
    }
    bool containsUnknownFields = false;
    FixInvalidEnums(requestResponse, containsUnknownFields);
    if (containsUnknownFields) {
        errors.push_back(GenerateError(TMegamindPrepared::TError::R_CONTAINS_UNKNOWN_FIELDS,
                                       TStringBuilder{} << NYT::NodeToYsonString(Row["request"]) << " "
                                                        << NYT::NodeToYsonString(Row["response"]) << " "
                                                        << NYT::NodeToYsonString(Row["analytics_info"]) << " "
                                                        << NYT::NodeToYsonString(Row["quality_storage"]),
                                       uuid, requestId, messageId));
    }
    requestResponse.SetTimestampLogMs(
        requestResponse.GetSpeechKitRequest().GetRequest().GetAdditionalOptions().GetServerTimeMs());

    res.RequestResponse = requestResponse;
    return res;
}

TMaybe<TString> TMegamindLogsParser::ParseUuid(const NYT::TNode& row) {
    auto uuid = MaybeStringFromYson(row[UUID]);
    if (uuid) {
        return NormalizeUuid(*uuid);
    }
    return uuid;
}

TMaybe<TString> TMegamindLogsParser::ParseRequestId(const NYT::TNode& row) {
    return MaybeStringFromYson(row[REQUEST_ID]);
}

TMaybe<TString> TMegamindLogsParser::ParseResponseId(const NYT::TNode& row) {
    if (row[RESPONSE_KEY].IsMap() && row[RESPONSE_KEY][HEADER_KEY].IsMap() &&
        row[RESPONSE_KEY][HEADER_KEY].HasKey(RESPONSE_ID)) {
        return MaybeStringFromYson(row[RESPONSE_KEY][HEADER_KEY][RESPONSE_ID]);
    }
    return {};
}

TMaybe<TString> TMegamindLogsParser::ParseMessageId(const NYT::TNode& row) {
    const TStringBuf REF_MESSAGE_ID_KEY = "ref_message_id";
    if (row[RESPONSE_KEY].IsMap() && row[RESPONSE_KEY][HEADER_KEY].IsMap() &&
        row[RESPONSE_KEY][HEADER_KEY].HasKey(REF_MESSAGE_ID_KEY)) {
        return MaybeStringFromYson(row[RESPONSE_KEY][HEADER_KEY][REF_MESSAGE_ID_KEY]);
    }
    return {};
}

TMegamindPrepared::TError TMegamindLogsParser::GenerateError(const TMegamindPrepared::TError::EReason reason,
                                                             const TString& message, const TMaybe<TString>& uuid,
                                                             const TMaybe<TString>& requestId,
                                                             const TMaybe<TString>& messageId) {
    TMegamindPrepared::TError error;
    error.SetProcess(TMegamindPrepared::TError::P_MEGAMIND_REQUEST_RESPONSE_MAPPER);
    error.SetReason(reason);
    error.SetMessage(message);
    if (uuid) {
        error.SetUuid(*uuid);
    }
    if (messageId) {
        error.SetMessageId(*messageId);
    }
    if (requestId) {
        error.SetRequestId(*requestId);
    }
    if (auto setraceUrl = TryGenerateSetraceUrl({messageId, requestId, uuid})) {
        error.SetSetraceUrl(*setraceUrl);
    }
    if (auto setraceUrl = TryGenerateSetraceUrl({messageId, requestId, uuid})) {
        error.SetSetraceUrl(*setraceUrl);
    }
    return error;
}

} // namespace NAlice::NWonderlogs
