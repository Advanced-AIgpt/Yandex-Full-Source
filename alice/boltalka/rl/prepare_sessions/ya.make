PY3_PROGRAM()

OWNER(
    nzinov
    g:alice_boltalka
)

PY_MAIN(alice.boltalka.rl.prepare_sessions.prepare_sessions)

PY_SRCS(prepare_sessions.py)

PEERDIR(
    contrib/python/numpy
    yt/python/client
    alice/boltalka/py_libs/apply_nlg_dssm
)

END()
