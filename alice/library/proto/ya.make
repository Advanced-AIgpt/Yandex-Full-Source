LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/library/logger
    alice/protos/extensions
    contrib/libs/protobuf
    library/cpp/string_utils/base64
)

SRCS(
    proto.cpp
    protobuf.cpp
    proto_struct.cpp
    proto_adapter.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
