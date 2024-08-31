PY2_LIBRARY()

OWNER(
    vitvlkv
    g:hollywood
)

PEERDIR(
    alice/hollywood/scripts/graph_generator/library
)

PY_SRCS(
    __init__.py
    converter.py
    merger.py
    parser.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
