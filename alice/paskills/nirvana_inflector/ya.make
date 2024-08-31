OWNER(g:paskills)

PY2_PROGRAM()

PY_SRCS(
    inflector_wrapper.py
    MAIN main.py
)

PEERDIR(
    bindings/python/inflector_lib
    bindings/python/lemmer_lib
)

END()
