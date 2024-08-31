#include "helpers.h"

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/generic/guid.h>

namespace NAlice::NUniproxy::NHelpers {
    TJsonValue CreateMessageTemplate(TStringBuf ns, TStringBuf name) {
        TJsonValue message{};
        auto& event = message["event"];
        {
            auto& header = event["header"];
            header["namespace"] = ns;
            header["name"] = name;
            header["messageId"] = CreateGuidAsString();
        }
        event["payload"] = TJsonValue(EJsonValueType::JSON_MAP);
        return message;
    }

    bool CheckMapHasStringValue(const TJsonValue& map, TStringBuf key, TStringBuf expectedValue) {
        const TJsonValue* value;
        if (!map.GetValuePointer(key, &value)) {
            return false;
        }
        Y_ENSURE(value->IsString());
        return value->GetString() == expectedValue;
    }

    bool CheckMapHasStringValue(const TJsonValue& map, TStringBuf key, TStringBuf expectedValue,
                                TStringBuf& actualValue) {
        const TJsonValue* value;
        if (!map.GetValuePointer(key, &value)) {
            return false;
        }
        Y_ENSURE(value->IsString());
        actualValue = value->GetString();
        return actualValue == expectedValue;
    }

    bool CheckMapHasUIntValue(const TJsonValue& map, TStringBuf key, ui64 expectedValue) {
        const TJsonValue* value;
        if (!map.GetValuePointer(key, &value)) {
            return false;
        }
        Y_ENSURE(value->IsUInteger());
        return value->GetUInteger() == expectedValue;
    }

    const TString* GetMapStringValueOrNull(const TJsonValue& map, TStringBuf key) {
        const TJsonValue* value;
        if (!map.GetValuePointer(key, &value)) {
            return nullptr;
        }
        Y_ENSURE(value->IsString());
        return &(value->GetString());
    }

    TJsonValue ParseTextResponse(TStringBuf response) {
        TJsonValue responseJson;
        if (!ReadJsonTree(response, &responseJson, false)) {
            ythrow TUniproxyInteractionError() << "Failed to parse response";
        }
        return responseJson;
    }

    const TJsonValue* GetValueOrNull(const TJsonValue& json, TStringBuf key) {
        const TJsonValue* value;
        return json.GetValuePointer(key, &value) ? value : nullptr;
    }

    const TJsonValue* GetDirectiveOrNull(const TJsonValue& response) {
        return GetValueOrNull(response, TStringBuf("directive"));
    }

    const TJsonValue* GetHeaderOrNull(const TJsonValue& response) {
        return GetValueOrNull(response, TStringBuf("header"));
    }

    void ValidateDirectiveNamespaceNameAndRefMessageId(const TJsonValue& directive, TStringBuf ns, TStringBuf name,
                                                       TStringBuf refMessageId) {
        const auto* header = GetHeaderOrNull(directive);
        if (header == nullptr) {
            ythrow TUniproxyInteractionError() << "Invalid response. No header";
        }
        TStringBuf actual;
        if (!CheckMapHasStringValue(*header, TStringBuf("namespace"), ns, actual)) {
            ythrow TUniproxyInteractionError() << "Unexpected response. Expected namespace \"" << ns
                                               << "\" actual namespace \"" << actual << "\"";
        }
        if (!CheckMapHasStringValue(*header, TStringBuf("name"), name, actual)) {
            ythrow TUniproxyInteractionError() << "Unexpected response. Expected name \"" << name << "\" actual name \""
                                               << actual << "\"";
        }
        if (!CheckMapHasStringValue(*header, TStringBuf("refMessageId"), refMessageId)) {
            ythrow TUniproxyInteractionError() << "Unexpected response. Expected refMessageId \""
                                               << refMessageId << "\"";
        }
    }

    void ValidateNamespaceNameAndRefMessageId(const TJsonValue& responseJson, TStringBuf ns, TStringBuf name,
                                              TStringBuf refMessageId) {
        const auto* directive = GetDirectiveOrNull(responseJson);
        if (directive == nullptr) {
            ythrow TUniproxyInteractionError() << "Unexpected response. Expected directive";
        }
        ValidateDirectiveNamespaceNameAndRefMessageId(*directive, ns, name, refMessageId);
    }

    bool IsNamespaceNameInResponse(const TJsonValue& responseJson, TStringBuf ns, TStringBuf name) {
        const auto* directive = GetDirectiveOrNull(responseJson);
        if (directive == nullptr) {
            ythrow TUniproxyInteractionError() << "Unexpected response. Expected directive";
        }
        const auto* header = GetHeaderOrNull(*directive);
        if (header == nullptr) {
            ythrow TUniproxyInteractionError() << "Invalid response. No header";
        }
        return (CheckMapHasStringValue(*header, TStringBuf("namespace"), ns) &&
                CheckMapHasStringValue(*header, TStringBuf("name"), name));
    }

    TString GetMessageId(const TJsonValue& message) {
        return message["event"]["header"]["messageId"].GetString();
    }

    bool IsMessageToSkip(const TJsonValue& message) {
        const auto* directive = GetDirectiveOrNull(message);
        if (!directive) {
            return false;
        }
        const auto* header = GetHeaderOrNull(*directive);
        if (!header) {
            return false;
        }
        const auto isSystemNs = CheckMapHasStringValue(*header, TStringBuf("namespace"), TStringBuf("System"));
        const auto isPushName = CheckMapHasStringValue(*header, TStringBuf("name"), TStringBuf("Push"));
        // We need to filter TTS Timings directive since VOICESERV-4260
        const auto isTimingsName = CheckMapHasStringValue(*header, TStringBuf("name"), TStringBuf("Timings"));

        return (isSystemNs && isPushName) || isTimingsName;
    }

    void ValidateStreamControl(const TJsonValue& message, TStreamId streamId) {
        const auto* streamControl = GetValueOrNull(message, TStringBuf("streamcontrol"));
        if (streamControl == nullptr) {
            ythrow TUniproxyInteractionError() << "Unexpected response. Expected streamcontrol, but got:" << message;
        }
        if (!CheckMapHasUIntValue(*streamControl, TStringBuf("streamId"), streamId)) {
            ythrow TUniproxyInteractionError() << "Unexpected streamId. Expected " << streamId;
        }
        if (!CheckMapHasUIntValue(*streamControl, TStringBuf("reason"), 0)) {
            ythrow TUniproxyInteractionError() << "Unexpected reason. Expected 0";
        }
        if (!CheckMapHasUIntValue(*streamControl, TStringBuf("action"), 0)) {
            ythrow TUniproxyInteractionError() << "Unexpected action. Expected 0";
        }
    }

    const TJsonValue& GetDirectivePayload(const TJsonValue& directive) {
        const auto* payload = GetValueOrNull(directive, TStringBuf("payload"));
        if (payload == nullptr) {
            ythrow TUniproxyInteractionError() << "Unexpected response. directive should have payload";
        }
        return *payload;
    }

    bool GetPayloadIsTrash(const TJsonValue& payload) {
        const auto* istrash = GetValueOrNull(payload, TStringBuf("isTrash"));
        if (istrash == nullptr) {
            return false;
        }
        return istrash->GetBooleanSafe();
    }

    bool IsEmptyHypothesis(const TJsonValue::TArray& array) {
        return array.size() == 1 && array[0]["words"].GetArray().empty();
    }

    TString AddCgi(const TString& url, const TVector<std::pair<TString, TString>>& params) {
        TStringBuf sanitizedUrl, query, fragment;
        SeparateUrlFromQueryAndFragment(url, sanitizedUrl, query, fragment);
        TCgiParameters cgis;
        cgis.ScanAddAll(query);
        for (auto& param : params) {
            cgis.InsertUnescaped(param.first, param.second);
        }
        return sanitizedUrl + TString("?") + cgis.Print() + fragment;
    }
}
