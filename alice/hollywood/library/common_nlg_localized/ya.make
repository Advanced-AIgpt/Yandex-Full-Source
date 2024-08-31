LIBRARY()

OWNER(g:hollywood alexanderplat)

COMPILE_NLG(
    NLG_COMPILER_LOCALIZED_MODE

    card_macros.nlg
    common.nlg
    error.nlg
    json_macros.nlg
    macros.nlg
    suggests.nlg
)

END()
