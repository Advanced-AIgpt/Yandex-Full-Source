LIBRARY()

OWNER(g:wonderlogs)

SRCS(
    megamind.cpp
    uniproxy.cpp
    wonderlogs.cpp
)

PEERDIR(
    alice/wonderlogs/protos
    alice/wonderlogs/library/common
    alice/wonderlogs/library/parsers/internal
    alice/library/json
    alice/megamind/api/request
    alice/megamind/protos/common/required_messages
    library/cpp/json/yson
)

END()

RECURSE_FOR_TESTS(ut)
