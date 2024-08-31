PY3_PROGRAM()

OWNER(
    samoylovboris
    g:alice_quality
)

PEERDIR(
    contrib/python/click
    yt/python/client
)

PY_SRCS(
    main.py
)

PY_MAIN(alice.begemot.tools.ammo_from_mm_logs.main)

END()

