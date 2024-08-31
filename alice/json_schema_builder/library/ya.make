PY3_LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/library/tool_log
    alice/library/python/code_generator
    bindings/python/inflector_lib
)

PY_SRCS(
    __init__.py
    cpp_generator.py
    errors.py
    nodes.py
    parser.py
    protobuf_generator.py
    util.py
    visitor.py
)

END()

RECURSE_FOR_TESTS(
    ut
)
