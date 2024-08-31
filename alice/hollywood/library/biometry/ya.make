LIBRARY()

OWNER(
    g:hollywood
    vitvlkv
)

PEERDIR(
    alice/hollywood/library/environment_state
    alice/hollywood/library/framework
    alice/hollywood/library/request

    alice/library/biometry
    alice/library/logger

    alice/protos/endpoint/capabilities/bio
)

SRCS(
    biometry_delegate.cpp
    client_biometry.cpp
)

END()

RECURSE_FOR_TESTS(ut)
