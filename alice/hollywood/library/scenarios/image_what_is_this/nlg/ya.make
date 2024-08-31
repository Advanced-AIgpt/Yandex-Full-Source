LIBRARY()

OWNER(
    polushkin
    g:cv-dev
)

COMPILE_NLG(
    image_what_is_this_ru.nlg
    image_what_is_this_cards_ru.nlg
)

PEERDIR(
    alice/hollywood/library/common_nlg
    alice/hollywood/library/scenarios/search/nlg
)

END()
