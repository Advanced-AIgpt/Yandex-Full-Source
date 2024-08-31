LIBRARY()

OWNER(
    deemonasd
    g:hollywood
    g:alice_boltalka
)

PEERDIR(
    alice/hollywood/library/scenarios/general_conversation/proto
)

SRCS(
    aggregated_reply_wrapper.cpp
    consts.cpp
    entity.cpp
    flags.cpp
)

END()
