LIBRARY()

OWNER(g:wonderlogs)

SRCS(
    dialogs.cpp
    megamind.cpp
    uniproxy.cpp
)

PEERDIR(
    alice/wonderlogs/library/common
    alice/wonderlogs/protos
    alice/wonderlogs/sdk/utils
    alice/megamind/protos/analytics
    alice/megamind/protos/common
    library/cpp/yson/node
)

END()

RECURSE_FOR_TESTS(ut)
