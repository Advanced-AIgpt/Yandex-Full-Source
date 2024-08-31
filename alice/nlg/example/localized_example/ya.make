LIBRARY()

OWNER(g:alice)

COMPILE_NLG(
    NLG_COMPILER_LOCALIZED_MODE

    simple_phrase.nlg
)

END()

RECURSE_FOR_TESTS(
    ut
)
