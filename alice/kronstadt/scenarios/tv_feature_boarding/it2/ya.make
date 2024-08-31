PY3TEST()

OWNER(
    g:smarttv
)

SIZE(MEDIUM)
DEPENDS(${ARCADIA_ROOT}/alice/kronstadt/scenarios/tv_feature_boarding/it2/shard)

PEERDIR(
    alice/megamind/protos/common
)

INCLUDE(${ARCADIA_ROOT}/alice/kronstadt/it2/common.inc)

TEST_SRCS(
    test.py
)

END()

RECURSE(shard)
