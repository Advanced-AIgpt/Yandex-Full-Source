import os
import socket
import random
from .utils import read_json, write_json, deepupdate, get_at


def _underscore_to_camel(x):
    ret = ""

    up = False
    for l in x:
        if l == "_":
            up = True
        else:
            ret += l.upper() if up else l
            up = False

    return ret


def _transform_graph_config(d, only_childs=False):
    ret = {}

    for k, v in d.items():
        if not only_childs:
            k = _underscore_to_camel(k)
        if isinstance(v, dict):
            are_node_names = k in ("nodeDeps", "nodes")
            v = _transform_graph_config(v, only_childs=are_node_names)
        ret[k] = v

    return ret


# -------------------------------------------------------------------------------------------------
def _resolve_yp_es(cluster_name, endpoint_set_id):
    from infra.yp_service_discovery.python.resolver.resolver import Resolver as YpResolver
    from infra.yp_service_discovery.api import api_pb2 as YpResolverApi

    request = YpResolverApi.TReqResolveEndpoints()
    request.cluster_name = cluster_name
    request.endpoint_set_id = endpoint_set_id

    resolver = YpResolver(client_name=f"test:{socket.gethostname()}", timeout=5)
    result = resolver.resolve_endpoints(request)

    instances = []
    for ep in result.endpoint_set.endpoints:
        if not ep.ready:
            continue
        instances.append(
            {"host": ep.fqdn, "ip": f"[{ep.ip6_address}]" if ep.ip6_address else ep.ip4_address, "port": ep.port}
        )
    return instances


# -------------------------------------------------------------------------------------------------
def _resolve_backend(settings):
    resolve_spec = settings.get("resolve_spec", {})
    if "slb" in resolve_spec:
        return [{"host": resolve_spec["slb"][0], "port": settings["port"]}]

    if "yp" in resolve_spec:
        cluster_name, endpoint_set_id = resolve_spec["yp"][0].split("/", maxsplit=1)
        return _resolve_yp_es(cluster_name, endpoint_set_id)

    if "nanny" in resolve_spec:
        raise NotImplementedError("resolve backend's instances by 'nanny'")

    raise NotImplementedError("unknown way to resolve backend's instances")


# -------------------------------------------------------------------------------------------------
class Graph:
    @classmethod
    def list_graphs(cls, vertical_dir):
        for fname in os.listdir(vertical_dir):
            if fname.startswith("_") or (not fname.endswith(".json")):
                continue
            yield cls(path=os.path.join(vertical_dir, fname))

    def __init__(self, path):
        self._path = path
        self._config = None

    def __str__(self):
        return f"graph({self.name})"

    def __repr__(self):
        return f"graph({self.name}, path={self._path})"

    def _load(self):
        self._config = read_json(self.path)

    @property
    def path(self):
        return self._path

    @property
    def file_name(self):
        return os.path.basename(self._path)

    @property
    def name(self):
        return self.file_name[:-5]  # without ".json"

    @property
    def config(self):
        if self._config is None:
            self._load()
        return self._config

    def backends(self, backends_dir):
        for _, node in self.config["settings"]["nodes"].items():
            if node.get("node_type", node.get("nodeType")) in ("TRANSPARENT", "TRANSPARENT_STREAMING", "EMBED"):
                continue
            name = node.get("backend_name", node.get("backendName"))
            try:
                yield Backend.get(name, backends_dir)
            except FileNotFoundError:
                raise FileNotFoundError(f"config for '{name}' in '{backends_dir}' (needed by {self})")

    def save(self, path=None, dir=None, horizon_format=True):
        if path is None:
            path = self._path if (dir is None) else os.path.join(dir, self.file_name)
        elif dir is not None:
            raise RuntimeError("path and dir can't be used simultaneously")

        cfg = _transform_graph_config(self.config["settings"]) if horizon_format else self.config
        write_json(cfg, path)


# -------------------------------------------------------------------------------------------------
class Backend:
    @classmethod
    def get(cls, name, backends_dir):
        fname = f"{name}.json"

        path = os.path.join(backends_dir, fname)
        if os.path.isfile(path):
            return cls(name, path, backends_dir)

        if "__" in fname:
            path = os.path.join(backends_dir, *fname.split("__", maxsplit=1))
            if os.path.isfile(path):
                return cls(name, path, backends_dir)

        raise FileNotFoundError(f"config for '{name}' in '{backends_dir}'")

    def __init__(self, name, path, backends_dir):
        self._name = name
        self._path = path
        self._backends_dir = backends_dir
        self._parent = None
        self._config = None
        self._instances = {}  # resolved instances for different configurations
        self._settings = {}  # assembled settings for different configurations

    def __str__(self):
        return f"backend({self._name})"

    def __repr__(self):
        return f"backend({self._name}, path={self._path})"

    def _load(self):
        self_config = read_json(self._path)
        parent_name = self_config.pop("inherits", None)
        if parent_name is not None:
            parent_config = Backend.get(parent_name, self._backends_dir).config
            self_config = deepupdate(parent_config, self_config)
        self._config = self_config

    @property
    def path(self):
        return self._path

    @property
    def file_name(self):
        return f"{self._name}.json"

    @property
    def name(self):
        return self._name

    @property
    def config(self):
        if self._config is None:
            self._load()
        return self._config

    def has_configuration(self, configuration):
        return get_at(self.config, "locations", "backends", configuration) is not None

    def get_transport(self, configuration):
        return self.get_settings(configuration)["transport"]

    def get_protocol(self, configuration):
        return self.get_settings(configuration)["protocol"]

    def get_settings(self, configuration):
        settings = self._settings.get(configuration)
        if settings is None:
            settings = self.config.get("default_settings", {})
            deepupdate(settings, get_at(self.config, "locations", "backends", configuration, default={}))
            self._settings[configuration] = settings
        return settings

    def get_http_url(self, configuration):
        if self.get_transport(configuration) != "NEH":
            raise RuntimeError("Not HTTP backend")

        instances = self.get_instances(configuration)
        instance = instances[random.randint(0, len(instances) - 1)]
        host = instance["host"]
        port = instance["port"]
        schema = self.get_protocol(configuration)
        if schema in ("post", "full"):
            schema = "http"
        return f"{schema}://{host}:{port}"

    def get_instances(self, configuration):
        instances = self._instances.get(configuration)
        if instances is None:
            instances = _resolve_backend(self.get_settings(configuration))
            self._instances[configuration] = instances
        return instances


# -------------------------------------------------------------------------------------------------
class VerticalSettings:
    def __init__(self, vertical_path):
        self._path = vertical_path
        self._config = None
        self._required_graphs = None

    def __str__(self):
        return "verticalSettings"

    def __repr__(self):
        return f"verticalSettings(path={self._path})"

    @property
    def settings_path(self):
        return os.path.join(self._path, "_vertical_settings.json")

    @property
    def required_graphs_path(self):
        return os.path.join(self._path, "_required_graphs.txt")

    @property
    def config(self):
        if self._config is None:
            self._config = read_json(self.settings_path)
        return self._config

    @property
    def required_locations(self):
        return self.config.get("requiredLocations", [])

    @property
    def required_graphs(self):
        if self._required_graphs is None:
            with open(self.required_graphs_path, "r") as f:
                self._required_graphs = [l.strip() for l in f.readlines()]
        return self._required_graphs
