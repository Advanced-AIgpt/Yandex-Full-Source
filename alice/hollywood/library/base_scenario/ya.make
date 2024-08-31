LIBRARY()

OWNER(
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/context
    alice/hollywood/library/global_context
    alice/hollywood/library/nlg
    alice/hollywood/library/resources
    alice/hollywood/library/util
    alice/hollywood/protos
    apphost/api/service/cpp
)

SRCS(
    scenario.cpp
)

END()
