#include "formatting.h"

#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/json/json_reader.h>

namespace NRTLog {
    using namespace google::protobuf;
    using namespace NJson;

    void StripStringFields(TJsonValue& jsonValue, ui64& strippedBytesCount) {
        static const size_t MAX_STRING_PROPERTY_SIZE = 1000000;

        if (jsonValue.GetType() == JSON_STRING) {
            const size_t size = jsonValue.GetString().size();
            if (size > MAX_STRING_PROPERTY_SIZE) {
                static const TString comment = "... (value stripped)";
                const size_t newSize = MAX_STRING_PROPERTY_SIZE - comment.size();
                jsonValue.SetValue(jsonValue.GetString().substr(0, newSize) + comment);
                strippedBytesCount += (size - newSize);
            }
        } else if (jsonValue.GetType() == JSON_MAP) {
            for (const auto& p: jsonValue.GetMap()) {
                StripStringFields(jsonValue[p.first], strippedBytesCount);
            }
        } else if (jsonValue.GetType() == JSON_ARRAY) {
            for (size_t i = 0; i < jsonValue.GetArray().size(); ++i) {
                StripStringFields(jsonValue[i], strippedBytesCount);
            }
        }
    }

    TFormattedRTLogEvent FormatRTLogEvent(const Message& message) {
        TFormattedRTLogEvent result;

        {
            NProtobufJson::TProto2JsonConfig config;
            config
                .SetFormatOutput(true)
                .SetEnumMode(NProtobufJson::TProto2JsonConfig::EnumName)
                .SetMapAsObject(true);

            NProtobufJson::Proto2Json(message, result.Value, config);
        }
        if (const NJson::TJsonValue::TMapType* fieldsMap; NJson::GetMapPointer(result.Value, "Fields", &fieldsMap)) {
            NJson::TJsonValue::TMapType reformattedFields;
            for (const auto& p: *fieldsMap) {
                TString jsonFieldValueStr;
                if (!p.second.GetString(&jsonFieldValueStr)) {
                    continue;
                }
                const auto it = fieldsMap->find(p.first + "__format");
                if (it == fieldsMap->end()) {
                    continue;
                }
                if (it->second != "json") {
                    continue;
                }
                NJson::TJsonValue jsonFieldValue;
                if (!NJson::ReadJsonTree(jsonFieldValueStr, &jsonFieldValue)) {
                    continue;
                }
                Y_VERIFY(reformattedFields.insert({p.first, std::move(jsonFieldValue)}).second);
            }
            if (!reformattedFields.empty()) {
                NJson::TJsonValue* fieldsJsonValue;
                if (result.Value.GetValuePointer("Fields", &fieldsJsonValue)) {
                    for (auto& p: reformattedFields) {
                        fieldsJsonValue->InsertValue(p.first, std::move(p.second));
                    }
                }
            }
        }

        StripStringFields(result.Value, result.StrippedBytesCount);

        return result;
    }
}
