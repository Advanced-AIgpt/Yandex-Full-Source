OWNER(
    g-kostin
    g:alice
)

PY3TEST()

PEERDIR(
    contrib/python/attrs
    alice/gamma/sdk/python/gamma_sdk/testing
    alice/gamma/skills/guess_animal_game/game
)

TEST_SRCS(
    test_game.py
)

END()
