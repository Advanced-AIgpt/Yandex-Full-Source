PY3_LIBRARY()
OWNER(g:voicetech-infra)

PY_SRCS(
    __init__.py
    func.py
)

END()

RECURSE_FOR_TESTS(ut)