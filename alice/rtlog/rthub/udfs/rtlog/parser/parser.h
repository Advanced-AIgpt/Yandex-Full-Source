#pragma once

#include <alice/rtlog/rthub/protos/rtlog.pb.h>

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <ydb/library/yql/public/udf/udf_counter.h>

#include <library/cpp/eventlog/evdecoder.h>

#include <util/folder/path.h>
#include <util/generic/cast.h>
#include <util/stream/mem.h>
#include <util/system/file.h>

using namespace NProtoBuf;

namespace NRTLog {

using namespace NRTLog;

struct TEventsParserCounters {
    NKikimr::NUdf::TCounter ErrorsCount;
    NKikimr::NUdf::TCounter BadUtf8BytesCount;
    NKikimr::NUdf::TCounter BadUtf8MessagesCount;
    NKikimr::NUdf::TCounter BadReqIdCount;
};

class TEventsParser final {
public:
    explicit TEventsParser(const TEventsParserCounters& counters);

    TVector<NRTLog::TRecord> Parse(const TStringBuf& chunk, bool saveBinaryFrame) const;

private:
    mutable TEventsParserCounters Counters;
};

} // namespace NRTLog
