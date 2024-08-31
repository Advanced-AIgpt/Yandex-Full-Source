PY23_LIBRARY()

OWNER(g:jupyter-cloud)

PEERDIR(
    contrib/python/ipykernel
    library/python/svn_version
)

PY_SRCS(
    __init__.py
    kernel.py
    complete.py
    log.py
)

IF(PYTHON3)
PEERDIR(
    contrib/python/PyYAML
    contrib/python/papermill
    contrib/python/nbconvert
    contrib/python/tornado
)
PY_SRCS(
    run.py
)
ENDIF()


END()
