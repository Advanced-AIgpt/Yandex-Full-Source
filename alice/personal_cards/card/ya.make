LIBRARY()

OWNER(
    g:personal-cards
)

PEERDIR(
    alice/bass/libs/logging

    library/cpp/json
)

SRCS(
    card_request.cpp
    utils.cpp
)

END()
