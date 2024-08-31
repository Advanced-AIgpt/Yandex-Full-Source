PY3_LIBRARY()

OWNER(g:jupyter-cloud)

VERSION(0.1.4+patched.0)

LICENSE(BSD)

# consul and etcd is commented out to not include dependency on consul.io service

PEERDIR(
#    contrib/python/aiohttp
    contrib/python/escapism
#    contrib/python/etcd3
    contrib/python/jupyterhub
    contrib/python/passlib
#    contrib/python/python-consul
#    contrib/python/toml
    contrib/python/aioredis/aioredis-1
)

PY_SRCS(
    TOP_LEVEL
    jupyterhub_traefik_proxy/__init__.py
    jupyterhub_traefik_proxy/_version.py
#    jupyterhub_traefik_proxy/consul.py
#    jupyterhub_traefik_proxy/etcd.py
    jupyterhub_traefik_proxy/install.py
    jupyterhub_traefik_proxy/kv_proxy.py
    jupyterhub_traefik_proxy/proxy.py
    jupyterhub_traefik_proxy/redis.py
#    jupyterhub_traefik_proxy/toml.py
    jupyterhub_traefik_proxy/traefik_utils.py
)

RESOURCE_FILES(
    PREFIX jupytercloud/contrib/jupyterhub-traefik-proxy/
    .dist-info/METADATA
    .dist-info/entry_points.txt
    .dist-info/top_level.txt
)

END()
