PY23_LIBRARY()

OWNER(
    kseniial
    g:alice_quality
)

PY_REGISTER(granet)

PEERDIR(
    alice/nlu/granet/lib
    
    alice/nlu/py_libs/granet/util/lib
)

SRCS(
    granet.pyx
)

END()

RECURSE_FOR_TESTS(
	data
    test
)
