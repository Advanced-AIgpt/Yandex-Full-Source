LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    storage.cpp
)

PEERDIR(
    alice/matrix/library/ydb

    alice/protos/api/matrix

    library/cpp/protobuf/interop
)

END()
