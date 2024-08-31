LIBRARY()

OWNER(alexanderplat g:megamind)

COMPILE_NLG(
    error_ar.nlg
    error_en.nlg
    error_ru.nlg
    error_tr.nlg
)

PEERDIR(
    alice/nlg/library/nlg
)

END()
