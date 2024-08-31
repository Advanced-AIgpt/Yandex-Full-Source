PY2_PROGRAM(nlg)

OWNER(g:alice)

PEERDIR(
    alice/nlg/library/python/codegen_tool
)

# needed for the COMPILE_NLG command
INDUCED_DEPS(h+cpp ${ARCADIA_ROOT}/alice/nlg/library/runtime/runtime.h)

PY_SRCS(
    __main__.py
)

END()
