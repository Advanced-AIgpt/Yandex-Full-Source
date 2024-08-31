import os
import logging
from .apphost_utils import Graph, VerticalSettings


class BackendPatcher:
    DEVNULL_INSTANCE = {"host": "devnull.yandex.net", "port": 80, "weight": 1, "min_weight": 1}

    def __init__(self, vertical, configuration, conf_dir):
        self.logger = logging.getLogger("BackendPatcher")
        self.vertical = vertical
        self.configuration = configuration
        self.conf_dir = conf_dir
        self.graphs_dir = os.path.join(conf_dir, "verticals", vertical)
        self.backends_dir = os.path.join(conf_dir, "backends")
        self._backends = None
        self._graphs = None
        self._vertical_settings = VerticalSettings(os.path.join(self.graphs_dir))
        self._required_graphs = None

    @property
    def settings(self):
        return self._vertical_settings

    @property
    def backends(self):
        if self._backends is None:
            backends = {}
            for graph in self.graphs.values():
                self.logger.debug(f"collect backends for {graph}...")
                for backend in graph.backends(self.backends_dir):
                    if backend.name in backends:
                        continue
                    backends[backend.name] = backend
                    self.logger.debug(f"add {backend} needed for {graph}")
            self._backends = backends
            self.logger.info(f"Collected {len(self._backends)} backends")

        return self._backends.values()

    @property
    def graphs(self):
        if self._graphs is None:
            self._graphs = {graph.name: graph for graph in Graph.list_graphs(self.graphs_dir)}
        return self._graphs

    def make_backends_patch(self, ignore_errors=False, overrides={}):
        patches = {}

        for backend in self.backends:
            if backend.file_name in patches:  # patch already exists
                continue

            try:
                patch = self._make_backend_patch(backend, overrides.get(backend.name))
                self.logger.debug(f"Patch for {backend} is made")
            except:
                self.logger.exception(f"Failed to make patch for {backend}")
                if not ignore_errors:
                    raise
                patch = {"instances": [self.DEVNULL_INSTANCE]}

            patches[backend.file_name] = patch

        return {"backends": patches}

    def _make_backend_patch(self, backend, endpoint=None):
        if endpoint is not None:
            instances = [{"host": endpoint[0], "port": endpoint[1]}]
        else:
            instances = backend.get_instances(self.configuration)

        return {"instances": [{**i, "weight": 1, "min_weight": 1} for i in instances]}
