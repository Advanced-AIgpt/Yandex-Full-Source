LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/megamind/library/util
    alice/megamind/protos/speechkit
    alice/library/network
    library/cpp/json
    library/cpp/http/misc
)

SRCS(
    error.cpp
)

END()

RECURSE_FOR_TESTS(ut)
