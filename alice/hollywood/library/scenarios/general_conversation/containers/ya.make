LIBRARY()

OWNER(
    deemonasd
    g:hollywood
    g:alice_boltalka
)

PEERDIR(
    alice/hollywood/library/scenarios/general_conversation/common
    alice/hollywood/library/scenarios/general_conversation/proto
    alice/hollywood/library/scenarios/general_conversation/rankers

    alice/hollywood/library/scenarios/suggesters/movie_akinator

    alice/hollywood/library/framework
    alice/hollywood/library/gif_card

    alice/begemot/lib/fixlist_index
    alice/begemot/lib/microintents_index

    alice/boltalka/emoji/applier

    alice/nlu/libs/binary_classifier

    library/cpp/dot_product
    library/cpp/iterator
    library/cpp/yaml/as
)

SRCS(
    general_conversation_fast_data.cpp
    general_conversation_resources.cpp
)

END()
