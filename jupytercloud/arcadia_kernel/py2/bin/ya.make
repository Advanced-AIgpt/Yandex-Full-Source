OWNER(g:jupyter-cloud)

PY2_PROGRAM(arcadia_default_py2)

PEERDIR(jupytercloud/arcadia_kernel/default)

PY_MAIN(jupytercloud.arcadia_kernel.lib.kernel)

RESOURCE(jupytercloud/arcadia_kernel/py2/ya.make ya.make)

END()
