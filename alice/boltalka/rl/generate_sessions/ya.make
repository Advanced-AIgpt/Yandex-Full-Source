PY3_PROGRAM()

OWNER(
    nzinov
    g:alice_boltalka
)

PY_SRCS(TOP_LEVEL generate_sessions.py)

PY_MAIN(generate_sessions)

PEERDIR(
    yt/python/client
    alice/boltalka/py_libs/apply_nlg_dssm
)

END()

RECURSE(data)
