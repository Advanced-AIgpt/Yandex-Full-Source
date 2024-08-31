# This should be INCLUDEd to ya.make of every it_grpc tests target

PEERDIR(
    alice/hollywood/library/python/testing/it_grpc
)

IF(NOT ${HOLLYWOOD_SHARD})
    SET(HOLLYWOOD_SHARD all)
ENDIF()

DEPENDS(
    alice/hollywood/library/python/testing/app_host
    apphost/tools/event_log_dump
)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/shards/${HOLLYWOOD_SHARD}/for_it2.inc)

DATA(
    arcadia/apphost/conf
)
