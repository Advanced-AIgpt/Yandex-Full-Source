LIBRARY()

OWNER(
    g:megamind
    alkapov
)

PEERDIR(
    alice/library/json
    alice/library/logger
    alice/megamind/protos/speechkit
    alice/megamind/protos/common/required_messages
    library/cpp/json
    library/cpp/string_utils/base64
)

SRCS(
    constructor.cpp
)

END()

RECURSE_FOR_TESTS(ut)
