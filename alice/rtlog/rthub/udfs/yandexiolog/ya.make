YQL_UDF(yandexiolog_udf)

YQL_ABI_VERSION(
    2
    8
    0
)

OWNER(
    valbon
    g:yandex_io
)

SRCS(
    parser.cpp
    udf.cpp
)

PEERDIR(
    alice/rtlog/protos
    alice/rtlog/rthub/protos
    contrib/libs/protoc
    infra/proto_logger/api
    ydb/library/yql/public/udf
    yql/library/protobuf_udf
)

END()

RECURSE_FOR_TESTS(
    ut
)
