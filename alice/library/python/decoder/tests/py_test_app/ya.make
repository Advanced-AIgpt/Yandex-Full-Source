PY3_PROGRAM(py_test_app)
OWNER(g:voicetech-infra)

PEERDIR(
    alice/library/python/decoder
)

PY_SRCS(
    MAIN main.py
)

END()
