PY3_PROGRAM()

PY_MAIN(vh3.cli:main)

PEERDIR(
    alice/beggins/internal/vh/operations

    nirvana/vh3/src
)

INCLUDE(${ARCADIA_ROOT}/nirvana/vh3/add_conf.inc)

END()
