PY3TEST()
OWNER(g:voicetech-infra)
# NO_LINT()

DEPENDS(alice/uniproxy/qloud_format)

TEST_SRCS(
    __init__.py
    common.py
    test_all.py
)

END()
