PY2TEST()

OWNER(
    g:hollywood
    vitvlkv
)

PEERDIR(
    alice/hollywood/library/python/ammolib
)

TEST_SRCS(
    test_converter.py
    test_merger.py
    test_parser.py
)

DATA(
    arcadia/alice/hollywood/library/python/ammolib/tests/data
)

END()
