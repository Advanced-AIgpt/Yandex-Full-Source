LIBRARY()

OWNER(
    yagafarov
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/common_nlg
)

COMPILE_NLG(
    cards/show_traffic_bass.nlg
    feedback_ru.nlg
    show_traffic_bass_ru.nlg
    show_traffic_bass__details_ru.nlg
)

END()