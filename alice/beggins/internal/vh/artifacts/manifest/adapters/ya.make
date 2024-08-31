PY3_PROGRAM()

OWNER(alkapov)

PY_MAIN(vh3.cli:main)

PY_SRCS(
    __init__.py
    ext.py
)

PEERDIR(
    nirvana/vh3/src
)

INCLUDE(${ARCADIA_ROOT}/nirvana/vh3/add_conf.inc)

END()
