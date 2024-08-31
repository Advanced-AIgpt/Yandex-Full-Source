YQL_UDF(scenariolog_udf)

YQL_ABI_VERSION(
    2
    8
    0
)

OWNER(
    norchine
    g:alice_iot
)

SRCS(
    parser.cpp
    udf.cpp
)

PEERDIR(
    alice/library/go/setrace/protos
    alice/rtlog/common
    alice/rtlog/common/eventlog
    alice/rtlog/protos
    alice/rtlog/rthub/protos
    apphost/lib/event_log/decl
    contrib/libs/protoc
    library/cpp/eventlog
    library/cpp/framing
    library/cpp/protobuf/json
    library/cpp/string_utils/base64
    robot/library/fork_subscriber
    robot/rthub/yql/generic_protos
    ydb/library/yql/minikql
    ydb/library/yql/public/udf
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_table
    yql/library/protobuf_udf
)

END()

RECURSE_FOR_TESTS(
    ut
)
