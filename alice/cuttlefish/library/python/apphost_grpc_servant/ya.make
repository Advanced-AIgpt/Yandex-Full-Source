PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    apphost/lib/grpc/protos
    apphost/api/service/python
)

PY_SRCS(__init__.py)

END()
