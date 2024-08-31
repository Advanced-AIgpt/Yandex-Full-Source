OWNER(the0)

PY3TEST()

TEST_SRCS(
    test.py
)

DEPENDS(
    search/wizard/data/wizard/CustomEntities
    alice/nlu/tools/entities/custom/build_automaton
)

PEERDIR(
    alice/nlu/py_libs/custom_entities
)

END()
