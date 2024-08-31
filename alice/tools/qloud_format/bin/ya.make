PY3_PROGRAM(qloud_format)

OWNER(g:voicetech-infra)

PY_SRCS(
    MAIN qloud_format.py
)

PEERDIR(
    alice/tools/qloud_format/lib
)

END()
