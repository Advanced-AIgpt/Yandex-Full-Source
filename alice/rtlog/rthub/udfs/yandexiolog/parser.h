#pragma once

#include <alice/rtlog/rthub/protos/rtlog.pb.h>

#include <ydb/library/yql/public/udf/udf_counter.h>

namespace NRTLog {
    enum class EEventFormat {
        BinaryEventLog = 1,
        Text = 2
    };

    class IYandexioLogParser {
    public:
        virtual ~IYandexioLogParser() = default;

        virtual TVector<TRecord> Parse(const TStringBuf& chunk) const = 0;
    };

    struct TYandexioLogParserCounters {
        NKikimr::NUdf::TCounter ErrorsCount;
        NKikimr::NUdf::TCounter StrippedBytesCount;
        NKikimr::NUdf::TCounter StrippedMessagesCount;
        NKikimr::NUdf::TCounter BadUtf8BytesCount;
        NKikimr::NUdf::TCounter BadUtf8MessagesCount;
    };

    THolder<IYandexioLogParser> MakeYandexioLogParser(const TYandexioLogParserCounters& counters);
}
