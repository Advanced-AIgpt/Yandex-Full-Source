PY3TEST()

OWNER(
    g:megamind
    dolgawin
)

PEERDIR(
    alice/megamind/api/utils/python
    alice/megamind/protos/speechkit
)

TEST_SRCS(
    test_converter.py
)

END()
