PY3_PROGRAM()

OWNER(
    g:hollywood
    vitvlkv
)

PEERDIR(
    alice/library/python/utils
    contrib/python/click
)

PY_SRCS(
    main.py
)

PY_MAIN(alice.hollywood.scripts.it_ammo_maker.main)

DEPENDS(
    apphost/tools/event_log_dump
)

END()
