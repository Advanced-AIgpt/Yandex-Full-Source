PY2TEST()

OWNER(dan-anastasev)

PEERDIR(
    alice/nlu/py_libs/syntax_parser
)

TEST_SRCS(
    test.py
)

DEPENDS(
    alice/nlu/py_libs/syntax_parser/model
)

DATA()

END()
