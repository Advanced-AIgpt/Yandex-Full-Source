PY3_PROGRAM(flow)

PY_MAIN(vh3.cli:main)

PEERDIR(
    alice/beggins/internal/vh/meta/graph

    nirvana/vh3/src
)

INCLUDE(${ARCADIA_ROOT}/nirvana/vh3/add_conf.inc)

END()
