LIBRARY()

OWNER(
    g:alice
)

PEERDIR(
    alice/library/network
    apphost/lib/proto_answers
    library/cpp/cgiparam
)

SRCS(
    request_builder.cpp
)

END()

RECURSE_FOR_TESTS(ut)
