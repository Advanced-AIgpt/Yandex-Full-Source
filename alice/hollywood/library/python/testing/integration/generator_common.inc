# This should be INCLUDEd to ya.make of every it/generator tests target

TAG(
    ya:manual
    ya:alice_run_request_generator
)

PEERDIR(
    alice/hollywood/library/python/testing/run_request_generator
)

DEPENDS(
    alice/hollywood/library/python/testing/app_host
    alice/megamind/scripts/run
    alice/megamind/server
    apphost/tools/event_log_dump
)

DATA(
    arcadia/alice/megamind/configs/common
    arcadia/alice/megamind/configs/dev
    arcadia/apphost/conf
)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/shards/all/for_it2.inc)
