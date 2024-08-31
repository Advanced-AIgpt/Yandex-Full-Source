PY23_LIBRARY()

OWNER(g:jupyter-cloud)

PEERDIR(jupytercloud/arcadia_kernel/lib)

# this is also PEERDIR
INCLUDE(${ARCADIA_ROOT}/jupytercloud/arcadia_kernel/ya.make.libra_noyql.inc)

END()
