PY23_LIBRARY()

OWNER(alexanderplat g:alice)

PY_REGISTER(alice.nlg.library.python.nlg_renderer.bindings)

SRCS(
    main.cpp
    py_nlg_renderer.cpp
)

PEERDIR(
    alice/nlg/library/nlg_renderer
    library/cpp/pybind
    library/cpp/json/yson
    library/cpp/yson/node
    library/python/yson_node
    contrib/python/py3c
)

END()
