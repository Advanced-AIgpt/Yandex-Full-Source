OWNER(g:jupyter-cloud)

PY3_PROGRAM(arcadia_default_py3)

PEERDIR(jupytercloud/arcadia_kernel/default)

PY_MAIN(jupytercloud.arcadia_kernel.lib.kernel)

RESOURCE(jupytercloud/arcadia_kernel/py3/ya.make ya.make)

END()
