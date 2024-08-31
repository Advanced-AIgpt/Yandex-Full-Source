PY3_LIBRARY()

PEERDIR(
    alice/beggins/internal/vh/meta/basket_generators
    alice/beggins/internal/vh/flows
    alice/beggins/internal/vh/operations
    alice/beggins/internal/vh/scripts
    contrib/python/pyaml
    library/python/resource
    nirvana/vh3/src
)

PY_SRCS(
    config.py
    evaluate.py
    scripts.py
)

INCLUDE(${ARCADIA_ROOT}/alice/beggins/data/meta/files.lst)

RESOURCE_FILES(
    ${BEGGINS_META_FILES}
)

END()
