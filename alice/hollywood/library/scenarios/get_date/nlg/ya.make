LIBRARY()

OWNER(g:hollywood)

PEERDIR(
    alice/hollywood/library/common_nlg
)

COMPILE_NLG(
    get_date_ar.nlg
    get_date_ru.nlg
)

END()
