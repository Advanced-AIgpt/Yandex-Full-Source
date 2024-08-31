LIBRARY()

OWNER(
    sparkle
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/library/logger
)

SRCS(
    environment_state.cpp
    endpoint.cpp
)

END()
