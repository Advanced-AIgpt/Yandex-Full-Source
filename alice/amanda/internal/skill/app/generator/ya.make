PY3_PROGRAM(generate)

OWNER(
    g:amanda
)

PEERDIR(
    alice/acceptance/modules/request_generator/lib
)

PY_SRCS(
    MAIN generate.py
)

END()