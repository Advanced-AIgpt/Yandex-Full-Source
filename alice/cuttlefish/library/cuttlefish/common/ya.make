LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    antirobot.cpp
    blackbox.cpp
    common_items.cpp
    datasync.cpp
    datasync_parser.cpp
    edge_flags.cpp
    exp_flags.cpp
    field_getters.cpp
    http_utils.cpp
    item_types.cpp
    metrics.cpp
    utils.cpp
)

PEERDIR(
    alice/cachalot/api/protos
    alice/cuttlefish/library/apphost
    alice/cuttlefish/library/protos
    alice/library/json

    apphost/lib/proto_answers
)

END()

RECURSE_FOR_TESTS(ut)
