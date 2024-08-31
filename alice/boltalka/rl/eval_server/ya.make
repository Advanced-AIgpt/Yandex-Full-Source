PY3_PROGRAM()

PEERDIR(
    contrib/python/numpy
    contrib/python/requests
)

PY_SRCS(
    main.py
)

PY_MAIN(
    alice.boltalka.rl.eval_server.main
)

END()
