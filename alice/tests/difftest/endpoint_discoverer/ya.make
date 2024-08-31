PY2_PROGRAM()

OWNER(sparkle)

PY_SRCS(
    __main__.py
)

PEERDIR(
    infra/yp_service_discovery/python/resolver
    infra/yp_service_discovery/api
)

END()
