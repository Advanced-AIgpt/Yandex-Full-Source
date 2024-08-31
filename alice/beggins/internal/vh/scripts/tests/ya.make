PY3TEST()

TEST_SRCS(
    test_scripter.py

    resources/fun_body_simple.py
    resources/fun_body_complex_begin.py
    resources/fun_body_complex_middle.py
    resources/fun_body_complex_end.py
)

PEERDIR(
    alice/beggins/internal/vh/scripts
)

END()
