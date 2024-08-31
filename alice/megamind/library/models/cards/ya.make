LIBRARY()

OWNER(
    alkapov
    g:megamind
)

PEERDIR(
    alice/megamind/library/models/interfaces
    contrib/libs/protobuf
)

SRCS(
    card_model.cpp
    div2_card_model.cpp
    div_card_model.cpp
    text_card_model.cpp
    text_with_button_card_model.cpp
)

END()
