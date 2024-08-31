UNITTEST_FOR(alice/library/proto)

OWNER(g:alice)

PEERDIR(
    alice/library/json
    alice/library/proto/ut/protos
    alice/library/unittest
)

SRCS(
    alice/library/proto/proto_ut.cpp
    alice/library/proto/protobuf_ut.cpp
    alice/library/proto/proto_struct_ut.cpp
    alice/library/proto/proto_adapter_ut.cpp
)

END()
