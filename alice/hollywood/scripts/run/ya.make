OWNER(g:hollywood)

PY3_PROGRAM(run-hollywood-bin)

IF(NOT ${HOLLYWOOD_SHARD})
    SET(HOLLYWOOD_SHARD all)
ENDIF()

DEPENDS(
    alice/hollywood/shards/${HOLLYWOOD_SHARD}/server
)

PEERDIR(
    alice/hollywood/library/config
    alice/library/python/server_runner
    alice/library/python/utils
    contrib/python/coloredlogs
)

PY_SRCS(
    MAIN
    run.py
)

END()
