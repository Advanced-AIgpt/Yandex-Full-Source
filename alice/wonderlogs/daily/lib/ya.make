LIBRARY()

OWNER(g:wonderlogs)

SRCS(
    asr_prepared.cpp
    basket.cpp
    dialogs.cpp
    differ.cpp
    expboxes.cpp
    json_wonderlogs.cpp
    megamind_prepared.cpp
    ttls.cpp
    uniproxy_prepared.cpp
    wonderlogs.cpp
)

PEERDIR(
    alice/library/censor/lib
    alice/library/field_differ/lib
    alice/library/json
    alice/megamind/library/handlers/utils
    alice/megamind/protos/common
    alice/wonderlogs/library/builders
    alice/wonderlogs/library/common
    alice/wonderlogs/library/parsers
    alice/wonderlogs/library/robot
    alice/wonderlogs/library/yt
    alice/wonderlogs/protos
    alice/wonderlogs/sdk/utils
    contrib/libs/protobuf
    kernel/geo
    library/cpp/binsaver
    library/cpp/json
    library/cpp/json/yson
    library/cpp/libgit2_wrapper
    library/cpp/protobuf/yt
    library/cpp/threading/future
    library/cpp/threading/future/subscription
    library/cpp/yson/node
    mapreduce/interface
    mapreduce/yt/interface
    mapreduce/yt/library/operation_tracker
    voicetech/library/proto_api
    voicetech/asr/logs/lib
)

END()

RECURSE_FOR_TESTS(
    ut
)
