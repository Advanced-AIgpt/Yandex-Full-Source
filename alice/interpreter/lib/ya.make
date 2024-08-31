PY3_LIBRARY(request_generator)

OWNER(zubchick)

PEERDIR(
    contrib/python/attrs
    contrib/python/Flask
    contrib/python/requests

    alice/megamind/protos/speechkit

)

PY_SRCS(
    app.py
    api/translate.py
    api/megamind.py
)

END()
