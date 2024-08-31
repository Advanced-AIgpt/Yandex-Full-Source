#pragma once

#include <library/cpp/json/json_value.h>
#include <optional>

namespace NAlice::NUniproxy::NHelpers {
    using namespace NJson;

    /// @brief The stream id type
    using TStreamId = ui32;

    /// @brief The exception class
    struct TUniproxyInteractionError: yexception {};

    TJsonValue CreateMessageTemplate(TStringBuf ns, TStringBuf name);
    bool CheckMapHasStringValue(const TJsonValue& map, TStringBuf key, TStringBuf expectedValue);
    bool CheckMapHasStringValue(const TJsonValue& map, TStringBuf key, TStringBuf expectedValue,
                                TStringBuf& actualValue);
    bool CheckMapHasUIntValue(const TJsonValue& map, TStringBuf key, ui64 expectedValue);

    const TString* GetMapStringValueOrNull(const TJsonValue& map, TStringBuf key);

    TJsonValue ParseTextResponse(TStringBuf response);
    const TJsonValue* GetValueOrNull(const TJsonValue& json, TStringBuf key);
    const TJsonValue* GetDirectiveOrNull(const TJsonValue& response);
    const TJsonValue* GetHeaderOrNull(const TJsonValue& response);

    void ValidateDirectiveNamespaceNameAndRefMessageId(const TJsonValue& directive, TStringBuf ns, TStringBuf name,
                                                       TStringBuf refMessageId);
    void ValidateNamespaceNameAndRefMessageId(const TJsonValue& responseJson, TStringBuf ns, TStringBuf name,
                                              TStringBuf refMessageId);
    void ValidateStreamControl(const TJsonValue& message, TStreamId streamId);

    bool IsNamespaceNameInResponse(const TJsonValue& responseJson, TStringBuf ns, TStringBuf name);
    bool IsMessageToSkip(const TJsonValue& message);

    TString GetMessageId(const TJsonValue& message);
    const TJsonValue& GetDirectivePayload(const TJsonValue& directive);
    bool GetPayloadIsTrash(const TJsonValue& payload);
    bool IsEmptyHypothesis(const TJsonValue::TArray& array);

    TString AddCgi(const TString& url, const TVector<std::pair<TString, TString>>& params);
}
