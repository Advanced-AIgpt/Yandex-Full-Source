PY3TEST()

OWNER(
    artemkorenev
    g:alice_boltalka
)

TEST_SRCS(
    test.py
)

PEERDIR(
    alice/boltalka/generative/training/data/tokenizer_py
)

DATA(
    sbr://1285332653
)

END()
