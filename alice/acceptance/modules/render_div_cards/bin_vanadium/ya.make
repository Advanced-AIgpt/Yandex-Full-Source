PY3_PROGRAM(render_div_cards)

OWNER(
    g:alice_downloaders
)

PEERDIR(
    contrib/python/attrs
    contrib/python/boto3
    contrib/python/botocore
    contrib/python/jsonschema
    contrib/python/simplejson

    alice/acceptance/modules/render_div_cards/lib
)

PY_SRCS(
    __main__.py
)

END()
