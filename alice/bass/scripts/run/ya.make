OWNER(g:bass g:megamind)

PY3_PROGRAM()

DEPENDS(alice/bass/bin)

PEERDIR(
    alice/library/python/server_runner
    alice/library/python/utils
    contrib/python/coloredlogs
)

PY_SRCS(
    MAIN
    run.py
)

END()