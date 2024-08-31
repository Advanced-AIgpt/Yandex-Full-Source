PY2TEST()

OWNER(dan-anastasev)

SIZE(MEDIUM)

PEERDIR(
    alice/nlu/py_libs/respect_rewriter
)

TEST_SRCS(
    test.py
)

DEPENDS(
    alice/nlu/py_libs/respect_rewriter/models
    alice/nlu/py_libs/syntax_parser/model
)

DATA()

REQUIREMENTS(ram:32)

END()
