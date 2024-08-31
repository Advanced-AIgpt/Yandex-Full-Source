PY3_PROGRAM(proxycli)

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __main__.py
)

PEERDIR(
    alice/uniproxy/tools/proxycli/library/proxycli
    alice/uniproxy/tools/proxycli/library/scenarios
)

END()
