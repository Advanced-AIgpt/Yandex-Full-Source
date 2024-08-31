LIBRARY()

OWNER(
    artemkoff
    g:marketinalice
)

PEERDIR(
    alice/hollywood/library/common_nlg
)

COMPILE_NLG(
    cards/common.nlg
    cards/gallery.nlg
    common.nlg
    common_ru.nlg
)

END()
