PY3_PROGRAM(yasm-client)

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PY_SRCS(__main__.py)

PEERDIR(
    alice/tools/yasm/client/library
)

END()
