PY3TEST()

OWNER(
    g:paskills
    penguin-diver
)

SIZE(SMALL)

TEST_SRCS(
    test_embedder.py
    test_index.py
    test_intent_resolver.py
)

PEERDIR(
    alice/paskills/penguinarium
)

END()
