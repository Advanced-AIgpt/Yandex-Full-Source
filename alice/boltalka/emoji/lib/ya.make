PY3_LIBRARY()

OWNER(
	nzinov
	g:alice_boltalka
)

PY_SRCS(main.py)

PEERDIR(
	alice/boltalka/py_libs/apply_nlg_dssm
	alice/boltalka/extsearch/query_basesearch/lib
	contrib/libs/tf/python
	contrib/python/numpy
)

END()

RECURSE(
	data
)