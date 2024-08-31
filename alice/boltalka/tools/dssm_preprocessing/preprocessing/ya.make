PY3_PROGRAM()

OWNER(g:alice_boltalka)

PY_SRCS(
    main.py
)

PY_MAIN(
    alice.boltalka.tools.dssm_preprocessing.preprocessing.main
)

PEERDIR(
    alice/boltalka/tools/dssm_preprocessing/preprocessing/lib
    yt/python/client
)

END()

RECURSE(lib/)
