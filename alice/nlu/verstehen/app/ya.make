PY2_PROGRAM(verstehen_app)

OWNER(
    artemkorenev
    g:alice_quality
)

PEERDIR(
    alice/nlu/verstehen/config
    alice/nlu/verstehen/granet_errors
    alice/nlu/verstehen/index
    alice/nlu/verstehen/markup
    alice/nlu/verstehen/preprocess
    # 3rd party
    contrib/python/Flask-RESTful
    library/python/resource
    library/python/ylog
)

PY_SRCS(
    NAMESPACE verstehen.app
    __main__.py
    __init__.py
    flask_server.py
    search_app.py
)

RESOURCE(
    static/web_interface.html resource/static/web_interface.html
    static/alice_favicon.png resource/static/alice_favicon.png
)

END()
