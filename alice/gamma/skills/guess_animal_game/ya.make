OWNER(
    g-kostin
    g:alice
)

PY3_PROGRAM()

PEERDIR(
    alice/gamma/sdk/python/gamma_sdk
    alice/gamma/skills/guess_animal_game/game
)

PY_SRCS(
    MAIN main.py
)

END()

RECURSE(
    game
)

RECURSE_FOR_TESTS(
    tests
)
