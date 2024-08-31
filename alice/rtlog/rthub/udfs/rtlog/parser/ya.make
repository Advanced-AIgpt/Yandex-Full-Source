LIBRARY()

YQL_ABI_VERSION(
    2
    8
    0
)

OWNER(g:megamind)

SRCS(
    parser.cpp
)

PEERDIR(
    alice/rtlog/common
    alice/rtlog/common/eventlog
    alice/rtlog/protos
    alice/rtlog/rthub/protos
    alice/rtlog/rthub/udfs/rtlog/util
    apphost/lib/event_log/decl
    contrib/libs/protoc
    library/cpp/eventlog
    library/cpp/protobuf/json
    library/cpp/string_utils/base64
    robot/library/fork_subscriber
    robot/rthub/yql/generic_protos
    search/begemot/status
    ydb/library/yql/minikql
    ydb/library/yql/public/udf
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_table
    yql/library/protobuf_udf
)

END()
