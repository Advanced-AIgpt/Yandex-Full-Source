PY3_LIBRARY()

OWNER(
    samoylovboris
)

PEERDIR(
    nirvana/vh3/src
)

PY_SRCS(
    custom_generator.py
    dc_miner.py
    dc_tom.py
    stable_generator.py
    tom_with_cache.py
)

END()
