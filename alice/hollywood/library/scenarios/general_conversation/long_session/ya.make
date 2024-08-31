LIBRARY()

OWNER(
    deemonasd
    g:alice_boltalka
    g:hollywood
)

PEERDIR(
    alice/bass/libs/ydb_helpers
    alice/hollywood/library/scenarios/general_conversation/proto
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_table
)

SRCS(
    long_session_client.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
