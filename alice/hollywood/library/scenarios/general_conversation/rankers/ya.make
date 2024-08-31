LIBRARY()

OWNER(
    alzaharov
    g:alice_boltalka
    g:hollywood
)

PEERDIR(
    alice/boltalka/memory/lstm_dssm/applier
    alice/hollywood/library/scenarios/general_conversation/proto
    alice/megamind/protos/common
    library/cpp/iterator
    library/cpp/json
)

SRCS(
    memory_ranker.cpp
)

END()
