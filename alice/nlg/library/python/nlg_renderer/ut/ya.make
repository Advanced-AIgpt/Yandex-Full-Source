PY3TEST()

OWNER(alexanderplat g:alice)

TEST_SRCS(
    test_nlg_renderer.py
)

PEERDIR(
    alice/nlg/library/python/nlg_renderer
    alice/nlg/library/python/nlg_renderer/ut/nlg
)

END()

RECURSE(
    nlg
)
