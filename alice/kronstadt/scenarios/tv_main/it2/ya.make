PY3TEST()

OWNER(
    g:paskills
)
SIZE(medium)
DEPENDS(${ARCADIA_ROOT}/alice/kronstadt/scenarios/tv_main/it2/shard)

INCLUDE(${ARCADIA_ROOT}/alice/kronstadt/it2/common.inc)
NO_CHECK_IMPORTS()

PEERDIR(
    alice/protos/data/droideka
    alice/protos/data/tv/home
)

#PY_SRCS(
#    conftest.py
#)

TEST_SRCS(
    conftest.py
    test_galleries.py
)

END()

RECURSE(shard)
