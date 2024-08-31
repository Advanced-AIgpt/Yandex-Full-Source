LIBRARY()

OWNER(
    artemkoff
    g:marketinalice
)

PEERDIR(
    alice/hollywood/library/scenarios/market/common/nlg
    alice/hollywood/library/common_nlg
)

COMPILE_NLG(
    how_much_ru.nlg
    common_ru.nlg
)

END()
