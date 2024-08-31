PY23_LIBRARY()

OWNER(g:alice_boltalka)

PY_SRCS(
	__init__.py
	#select_replies_from_tops.py
)

PEERDIR(
    nirvana/valhalla/src
    yt/python/client
)

END()