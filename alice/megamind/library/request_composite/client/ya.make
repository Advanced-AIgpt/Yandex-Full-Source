LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/library/client
    alice/library/experiments
    alice/megamind/library/experiments
    alice/megamind/protos/common
    alice/megamind/protos/speechkit
)

SRCS(
    client.cpp
)

END()
