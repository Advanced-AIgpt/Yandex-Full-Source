PY3TEST()

OWNER(
    tolyandex
    vitvlkv
    g:hollywood
    g:alice
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/generator_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/search/it
    alice/hollywood/library/scenarios/search/proto
)

TEST_SRCS(
    generator.py
    fixlist_generator.py
    push_generator.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/search/it/data
    arcadia/alice/hollywood/library/scenarios/search/it/data_fixlist
    arcadia/alice/hollywood/library/scenarios/search/it/data_push
)

END()
