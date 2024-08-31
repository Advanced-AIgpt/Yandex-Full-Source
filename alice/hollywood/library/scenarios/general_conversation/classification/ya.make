LIBRARY()

OWNER(
    alzaharov
    g:alice_boltalka
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/scenarios/general_conversation/candidates
    alice/hollywood/library/scenarios/general_conversation/common
    alice/hollywood/library/scenarios/general_conversation/proto

    alice/hollywood/library/scenarios/suggesters/movie_akinator

    alice/hollywood/library/framework
    alice/hollywood/library/request/utils
)

SRCS(
    frame_classifier.cpp
)

END()
