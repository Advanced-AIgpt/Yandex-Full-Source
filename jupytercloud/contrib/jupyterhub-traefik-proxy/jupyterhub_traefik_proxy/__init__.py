"""Traefik implementation of the JupyterHub proxy API"""

from .proxy import TraefikProxy  # noqa
from .kv_proxy import TKvProxy  # noqa
# from .etcd import TraefikEtcdProxy
# from .consul import TraefikConsulProxy  # consul is disabled in this contrib
# from .toml import TraefikTomlProxy
from .redis import TraefikRedisProxy  # noqa

from ._version import get_versions

__version__ = get_versions()["version"]
del get_versions
