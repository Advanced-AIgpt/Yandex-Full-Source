LIBRARY()

OWNER(g:alice_fun)

SRCS(
    field_differ.cpp
)

PEERDIR(
    alice/library/json

    alice/library/field_differ/protos

    contrib/libs/protobuf
)

END()

RECURSE_FOR_TESTS(ut)
