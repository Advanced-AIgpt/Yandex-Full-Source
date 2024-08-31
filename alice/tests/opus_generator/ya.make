PY3_PROGRAM()

OWNER(
    mihajlova
    g:alice
)

PEERDIR(
    alice/tests/opus_generator/library
    contrib/python/sh
    nirvana/valhalla/src
)

PY_SRCS(
    MAIN
    main.py
)

END()
