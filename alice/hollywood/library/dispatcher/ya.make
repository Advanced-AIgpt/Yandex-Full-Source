LIBRARY()

OWNER(
    akhruslan
    g:hollywood
)

SRCS(
    apphost.cpp
)

PEERDIR(
    alice/hollywood/library/config
    alice/hollywood/library/dispatcher/common_handles/hw_service_handles
    alice/hollywood/library/dispatcher/common_handles/scenario_handles
    alice/hollywood/library/dispatcher/common_handles/service_handles
    alice/hollywood/library/dispatcher/common_handles/split_web_search
    alice/hollywood/library/dispatcher/common_handles/util
    alice/hollywood/library/global_context
    alice/hollywood/library/framework
    alice/hollywood/library/metrics
    alice/hollywood/library/registry
    alice/hollywood/protos
    alice/library/json
    alice/library/logger
    alice/library/metrics
    alice/library/network
    alice/library/proto
    alice/library/version
    alice/megamind/protos/scenarios
    contrib/libs/protobuf
    library/cpp/uri
    library/cpp/neh
    library/cpp/sighandler
)

END()
