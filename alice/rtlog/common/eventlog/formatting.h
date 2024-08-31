#pragma once

#include <library/cpp/json/writer/json_value.h>

namespace google::protobuf {
    class Message;
}

namespace NRTLog {
    struct TFormattedRTLogEvent {
        NJson::TJsonValue Value;
        ui64 StrippedBytesCount;

        TFormattedRTLogEvent()
            : Value()
            , StrippedBytesCount(0)
        {
        }
    };

    TFormattedRTLogEvent FormatRTLogEvent(const google::protobuf::Message& message);
}
