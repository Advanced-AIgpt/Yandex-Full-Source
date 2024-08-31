PY23_TEST()

OWNER(g:jupyter-cloud)

SET(PREFIX jupytercloud/arcadia_kernel/tests/medium)

SIZE(MEDIUM)

PEERDIR(
    jupytercloud/arcadia_kernel/default
)

TEST_SRCS(
    ${PREFIX}/test_kernel.py
    ${PREFIX}/test_packages_integration.py
    ${PREFIX}/test_arcadia_info.py
    ${PREFIX}/conftest.py
)

IF(PYTHON2)
DEPENDS(jupytercloud/arcadia_kernel/py2)
ENDIF()

IF(PYTHON3)
DEPENDS(jupytercloud/arcadia_kernel/py3)
ENDIF()

END()
