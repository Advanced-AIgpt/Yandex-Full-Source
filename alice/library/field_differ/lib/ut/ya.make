UNITTEST()

OWNER(g:alice_fun)

PEERDIR(
    alice/library/unittest

    alice/library/field_differ/lib
)

SRCS(
    testing.proto
    field_differ_ut.cpp
)

END()
