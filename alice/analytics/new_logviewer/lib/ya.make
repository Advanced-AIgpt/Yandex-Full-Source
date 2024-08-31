PY3_LIBRARY()

OWNER(andreyshspb)

PEERDIR(
    contrib/python/attrs
    contrib/python/Flask
    contrib/python/Flask-WTF
    contrib/python/PyYAML
    contrib/python/WTForms
    library/python/deploy_formatter
    library/python/monlib
    library/python/statface_client
    yt/python/yt/clickhouse
    yt/python/yt/wrapper
)

PY_SRCS(
    search/logviewer_request.py
    search/logviewer_response.py
    search/logviewer_executor.py

    app/server.py
    app/forms.py
    app/logviewer_config.py
    app/metrics.py
)

END()
