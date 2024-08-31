LIBRARY()

OWNER(
    g:personal-cards
)

SRCS(
    push_cards_storage.cpp
)

PEERDIR(
    alice/personal_cards/config
    alice/personal_cards/protos

    ydb/public/sdk/cpp/client/ydb_scheme
    ydb/public/sdk/cpp/client/ydb_table

    library/cpp/json
)

END()
