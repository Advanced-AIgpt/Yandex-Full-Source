import os
import shutil
import logging
from .utils import read_json, write_json


# -------------------------------------------------------------------------------------------------
class HGraph:
    def __init__(self, path):
        self._path = path
        self._name = os.path.splitext(os.path.basename(self._path))[0]
        self._config = None

    def __str__(self):
        return f"graph({self._name})"

    def __repr__(self):
        return f"graph({self._name}, path={self._path})"

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
            self._config = read_json(self._path)
        return self._config


# -------------------------------------------------------------------------------------------------
class HBackend:
    def __init__(self, path):
        self._path = path
        self._name = os.path.splitext(os.path.basename(self._path))[0]
        self._config = None

    def __str__(self):
        return f"backend({self._name})"

    def __repr__(self):
        return f"backend({self._name}, path={self._path})"

    def save_to(self, path):
        if self._config is None:
            shutil.copyfile(self._path, path)
        else:
            write_json(self._config, path)

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
            self._config = read_json(self._path)
        return self._config

    @property
    def transport(self):
        return self.config["transport"]

    @property
    def protocol(self):
        return self.config["protocol"]

    def set_endpoint(self, endpoint):
        first_instance = self.config["instanceGroups"][0]["instances"][0]
        first_instance["host"] = endpoint[0]
        first_instance["port"] = str(endpoint[1])


# -------------------------------------------------------------------------------------------------
class HorizonData:
    def __init__(self, dir_path):
        self.logger = logging.getLogger("HorizonData")
        self._dir = dir_path
        self._backends = None
        self._graphs = None

    @property
    def path(self):
        return self._dir

    @property
    def backends_path(self):
        return os.path.join(self._dir, "backends")

    @property
    def graphs_path(self):
        return os.path.join(self._dir, "graphs")

    @property
    def backends(self):
        # dict: <backend name> -> HBackend
        if self._backends is None:
            backends = {}

            for fname in os.listdir(self.backends_path):
                if fname.startswith("_") or (not fname.endswith(".json")):
                    continue
                backend = HBackend(os.path.join(self.backends_path, fname))
                backends[backend.name] = backend

            self._backends = backends

        return self._backends

    @property
    def graphs(self):
        if self._graphs is None:
            graphs = {}

            for fname in os.listdir(self.graphs_path):
                if fname.startswith("_") or (not fname.endswith(".json")):
                    continue
                graph = HGraph(os.path.join(self.graphs_path, fname))
                graphs[graph.name] = graph

            self._graphs = graphs

        return self._graphs

    def save_to(self, path):
        os.makedirs(path)

        # save backends
        dst_backends_path = os.path.join(path, "backends")
        if self._backends is None:
            shutil.copytree(src=self.backends_path, dst=dst_backends_path)
        else:
            os.makedirs(dst_backends_path)
            for b in self._backends.values():
                b.save_to(os.path.join(dst_backends_path, b.file_name))

        # save graphs
        shutil.copytree(src=self.graphs_path, dst=os.path.join(path, "graphs"))
