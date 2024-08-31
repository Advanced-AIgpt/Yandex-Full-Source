# This should be INCLUDEd to ya.make of every it/runner tests target

PEERDIR(
    alice/hollywood/library/python/testing/integration
    alice/hollywood/library/python/testing/stubber
    alice/megamind/protos/scenarios
    contrib/python/requests
    library/python/vault_client
)

DEPENDS(
    alice/hollywood/library/python/testing/app_host
)

DATA(
    arcadia/apphost/conf/backends/ALICE
    arcadia/apphost/conf/verticals/ALICE
)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/shards/all/for_it2.inc)
INCLUDE(${ARCADIA_ROOT}/apphost/conf/verticals/ALICE/_alice_external_backends.inc)
