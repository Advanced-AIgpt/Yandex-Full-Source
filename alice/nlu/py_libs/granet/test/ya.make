PY2TEST()

OWNER(
	kseniial
	g:alice_quality
)

PEERDIR(
    contrib/python/pathlib2
    contrib/python/pandas
	alice/nlu/py_libs/granet
)

DEPENDS(
    alice/nlu/granet/tools/granet
)

SIZE(LARGE)
TAG(ya:fat)

DATA(
    arcadia/alice/nlu/data/ru/granet/
    arcadia/alice/nlu/py_libs/granet/data/alice/nlu/data/ru/test/pool/

    arcadia/alice/nlu/granet/tools/granet/
)

TEST_SRCS(
	comparison_test.py
)

END()