LIBRARY()
OWNER(g:voicetech-infra)

PEERDIR(
    library/cpp/json
    contrib/libs/rapidjson
)

SRCS(
    evcheck.cpp
    evcheck.h
    builder.cpp
    builder.h
    node.cpp
    node.h
    parser.cpp
    parser.h
    read_handler.h
    sax_callbacks.h
)

END()

RECURSE_FOR_TESTS(ut)
