PY3_PROGRAM(nanny_tool)

OWNER(g:voicetech-infra)

PY_SRCS(
    __main__.py
)

PEERDIR(
    alice/tools/nanny_tool/lib
)

END()
