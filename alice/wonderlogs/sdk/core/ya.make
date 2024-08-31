LIBRARY()

OWNER(g:wonderlogs)

SRCS(
    getters.cpp
)

PEERDIR(
    alice/megamind/protos/analytics
    alice/megamind/protos/speechkit
    alice/library/json
    dict/dictutil
    library/cpp/cgiparam
    library/cpp/json
    library/cpp/string_utils/url
    library/cpp/string_utils/quote
)

END()

RECURSE_FOR_TESTS(ut)
