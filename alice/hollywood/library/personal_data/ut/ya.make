UNITTEST_FOR(alice/hollywood/library/personal_data)

OWNER(g:alice)

PEERDIR(
    alice/hollywood/library/personal_data
    alice/library/client
    alice/library/client/protos
    alice/library/data_sync
    alice/library/unittest
)

SRCS(
    personal_data_ut.cpp
)

END()
