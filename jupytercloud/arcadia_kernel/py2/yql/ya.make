OWNER(g:jupyter-cloud)

YQL_PYTHON_UDF(arcadia_default_py2)

REGISTER_YQL_PYTHON_UDF(
    ADD_LIBRA_MODULES yes
)

PEERDIR(jupytercloud/arcadia_kernel/default_yql)

PY_MAIN(jupytercloud.arcadia_kernel.lib.kernel)

RESOURCE(jupytercloud/arcadia_kernel/py2/ya.make ya.make)

END()
