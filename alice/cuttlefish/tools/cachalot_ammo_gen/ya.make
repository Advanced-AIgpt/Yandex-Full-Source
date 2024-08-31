OWNER(g:voicetech-infra)

PY3_PROGRAM(cachalot-ammo-gen)

STYLE_PYTHON()

PY_SRCS(__main__.py)

PEERDIR(
    alice/cachalot/api/protos
)

END()
