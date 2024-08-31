PY3_PROGRAM(marker_tests)

OWNER(g:alice)

PEERDIR(
    contrib/python/attrs
    contrib/python/AttrDict
    contrib/python/click
    contrib/python/pytz
    contrib/python/PyYAML

    yt/python/client

    nirvana/valhalla/src
    alice/acceptance/cli/marker_tests/lib
)

PY_SRCS(
    config.py
    MAIN run.py
)

INCLUDE(${ARCADIA_ROOT}/alice/acceptance/cli/marker_tests/bin/data.inc)

END()
