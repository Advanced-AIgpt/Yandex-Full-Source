LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    cache.cpp
    service.h
    service.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/experiments
    alice/cuttlefish/library/logging

    # TODO (paxakor@) remove dependency after moving flags_json and laas to context_load
    alice/cuttlefish/library/cuttlefish/synchronize_state

    alice/cachalot/api/protos
    alice/iot/bulbasaur/protos/apphost
    alice/library/cachalot_cache
    alice/memento/proto
    alice/protos/api/meta
    library/cpp/string_utils/base64
    voicetech/library/messages

    apphost/api/service/cpp
    apphost/lib/proto_answers
    mssngr/router/lib/protos/registry
)

END()

RECURSE (
    client
)
