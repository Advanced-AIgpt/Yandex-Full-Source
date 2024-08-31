PY3_PROGRAM(solomon-client)

OWNER(g:voicetech-infra)

PY_SRCS(
    __main__.py
)

PEERDIR(
    alice/tools/solomon/client/library
)

END()
