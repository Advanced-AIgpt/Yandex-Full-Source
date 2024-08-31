PY3_LIBRARY()

OWNER(g:alice mihajlova)

PEERDIR(
    alice/library/python/utils
    devtools/ya/yalibrary/svn
    devtools/ya/yalibrary/yandex/sandbox
    contrib/python/retry
    library/python/vault_client
)

PY_SRCS(
    resource_environment.py
    runner.py
    vins_package.py
    __init__.py
)

END()
