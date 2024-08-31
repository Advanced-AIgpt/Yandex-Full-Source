PY2_LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/library/python/code_generator
    alice/library/tool_log
    alice/nlg/library/python/codegen/proto
    alice/vins/core/vins_core/nlg
    alice/vins/core/vins_core/utils
    contrib/python/future
)

PY_SRCS(
    __init__.py
    call.py
    cpp_compiler.py
    errors.py
    keyset_producer.py
    localized_builder.py
    nodes.py
    parser.py
    scope.py
    transformer.py
    visitor.py
)

END()

RECURSE_FOR_TESTS(
    ast_ut
    ut
)
