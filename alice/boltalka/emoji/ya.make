PY3_PROGRAM()

OWNER(
	nzinov
	g:alice_boltalka
)

PY_SRCS(main.py)

PY_MAIN(alice.boltalka.emoji.main)

PEERDIR(
	alice/boltalka/emoji/lib
)

END()

RECURSE(
	applier
	lib
)