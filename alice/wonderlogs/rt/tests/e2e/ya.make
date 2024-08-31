PY3TEST()

OWNER(g:wonderlogs)

TEST_SRCS(test.py)

PEERDIR(
    ads/bsyeti/big_rt/cli
    ads/bsyeti/big_rt/py_lib
    alice/wonderlogs/rt/processors/megamind_resharder/protos
    alice/wonderlogs/rt/processors/megamind_creator/protos
    alice/wonderlogs/rt/processors/uniproxy_resharder/protos
    alice/wonderlogs/rt/processors/uniproxy_creator/protos
    alice/wonderlogs/rt/processors/wonderlogs_creator/protos
    alice/wonderlogs/rt/protos
    contrib/python/protobuf
)

DEPENDS(
    alice/wonderlogs/rt/processors/megamind_creator/bin
    alice/wonderlogs/rt/processors/megamind_resharder/bin
    alice/wonderlogs/rt/processors/uniproxy_resharder/bin
    alice/wonderlogs/rt/processors/uniproxy_creator/bin
    alice/wonderlogs/rt/processors/wonderlogs_creator/bin
)

FROM_SANDBOX(
    2857940501
    OUT_NOAUTO
    megamind.json
    uniproxy.json
)

RESOURCE(
    megamind.json megamind.json
    uniproxy.json uniproxy.json
)

ENV(YT_TOKEN=lolkek)
ENV(SET_DEFAULT_LOCAL_YT_PARAMETERS=1)

INCLUDE(${ARCADIA_ROOT}/alice/wonderlogs/rt/tests/configs/resources.inc)
INCLUDE(${ARCADIA_ROOT}/mapreduce/yt/python/recipe/recipe_with_tablets.inc)

END()
