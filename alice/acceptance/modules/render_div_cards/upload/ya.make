PY3_PROGRAM()

OWNER(
    g:alice_downloaders
)

PEERDIR(
    contrib/python/attrs
    contrib/python/boto3
    contrib/python/botocore
    contrib/python/ijson
    contrib/python/jsonschema
    contrib/python/requests
    contrib/python/simplejson
)

PY_SRCS(
    __main__.py
)

END()
