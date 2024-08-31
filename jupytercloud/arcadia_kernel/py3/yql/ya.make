OWNER(g:jupyter-cloud)

YQL_PYTHON3_UDF(arcadia_default_py3)

REGISTER_YQL_PYTHON_UDF(
    NAME CustomPython3
    ADD_LIBRA_MODULES yes
)

PEERDIR(jupytercloud/arcadia_kernel/default_yql)

PY_MAIN(jupytercloud.arcadia_kernel.lib.kernel)

RESOURCE(jupytercloud/arcadia_kernel/py3/ya.make ya.make)

END()
