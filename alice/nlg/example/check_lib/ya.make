LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/nlg/example
)

COMPILE_NLG(
    date_phrases_ru.nlg
    datetime_exports_ru.nlg
    exports_ru.nlg
    nlgimport_ru.nlg
    simple_bottom_ru.nlg
    simple_middle_ru.nlg
    simple_top_ru.nlg
)

END()
