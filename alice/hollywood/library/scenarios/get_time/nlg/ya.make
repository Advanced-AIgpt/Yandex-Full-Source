LIBRARY()

OWNER(alexanderplat g:hollywood)

COMPILE_NLG(
    common_ar.nlg
    common_ru.nlg

    get_time_ar.nlg
    get_time_ru.nlg

    suggests_ar.nlg
    suggests_ru.nlg
)

PEERDIR(
    alice/hollywood/library/common_nlg
)

END()
