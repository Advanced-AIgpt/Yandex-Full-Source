OWNER(g:alice)

LIBRARY()

PEERDIR(
    library/cpp/cgiparam
    library/cpp/http/misc
    library/cpp/uri
)

SRCS(
    common.cpp
    headers.cpp
    request_builder.cpp
)

END()

RECURSE_FOR_TESTS(ut)
