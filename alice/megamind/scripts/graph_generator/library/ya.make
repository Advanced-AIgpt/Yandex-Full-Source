PY3_LIBRARY()

OWNER(akhruslan)

PEERDIR(
    alice/megamind/library/config/protos
    alice/megamind/library/config/scenario_protos
)

PY_SRCS(
    common.py
    generator.py
)

END()
