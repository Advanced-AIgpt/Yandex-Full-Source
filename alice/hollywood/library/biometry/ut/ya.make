UNITTEST_FOR(alice/hollywood/library/biometry)

OWNER(
    klim-roma
    g:hollywood
    g:alice
)

SRCS(
    alice/hollywood/library/biometry/client_biometry_ut.cpp
)

PEERDIR(
    alice/hollywood/library/biometry

    alice/megamind/protos/guest
    
    library/cpp/testing/unittest
    apphost/lib/service_testing
)

END()
