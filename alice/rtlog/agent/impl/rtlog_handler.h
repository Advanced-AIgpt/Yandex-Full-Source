#pragma once

#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace google::protobuf {
    class Message;
}

namespace NRTLogAgent {
    struct TLogItemLocation {
        ui32 SegmentTimestamp;
        ui32 Offset;
        ui32 Size;
    };

    struct TFieldSpec {
        TString Key;
        TString Value;
        ui32 EventIndex;
    };

    class IRTLogHandler {
    public:
        virtual ~IRTLogHandler() = default;

        virtual THolder<google::protobuf::Message> ParseForIndex(const char* data, const TLogItemLocation& location) = 0;

        virtual TString ParseForQuery(const TBuffer& data, const TMaybe<TFieldSpec>& fieldSpec, bool pretty) = 0;
    };

    THolder<IRTLogHandler> MakeRTLogHandler();
}
