PY3TEST()
OWNER(g:voicetech-infra)


DATA(
    arcadia/alice/library/python/decoder/tests/test123-opus.ogg
    arcadia/alice/library/python/decoder/tests/bear-vp9-opus.webm
)

DEPENDS(
    alice/library/python/decoder/tests/cpp_test_app
)

PEERDIR(
    alice/library/python/decoder
)

TEST_SRCS(
    common.py
    test_cpp_decoder.py
    test_py_decoder.py
)

END()
