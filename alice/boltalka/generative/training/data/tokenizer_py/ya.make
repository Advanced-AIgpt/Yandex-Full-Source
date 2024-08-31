PY3_LIBRARY()

OWNER(
    artemkorenev
    g:alice_boltalka
)

PY_REGISTER(tokenizer)

PEERDIR(
    alice/boltalka/generative/training/data/tokenizer
)

SRCS(
    tokenizer.pyx
)

END()
