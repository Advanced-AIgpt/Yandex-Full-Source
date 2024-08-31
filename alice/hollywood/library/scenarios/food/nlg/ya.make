LIBRARY()

OWNER(
    samoylovboris
    the0
    g:alice_quality
)

COMPILE_NLG(
    cart.nlg
    food_ru.nlg
    suggests_ru.nlg
)

END()

RECURSE_FOR_TESTS(
    ut
)
