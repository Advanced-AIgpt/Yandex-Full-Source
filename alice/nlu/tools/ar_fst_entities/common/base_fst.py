from abc import ABC, abstractmethod
from alice.nlu.tools.ar_fst_entities.common import utils
from pynini.export import export


class BaseFst(ABC):
    def __init__(self, name, dictionaries_path, path):
        self.name = name
        self.path = path
        self.dictionaries_path = dictionaries_path
        self.common = utils.CommonFsts(self.dictionaries_path)
        self.model = None

    @abstractmethod
    def create_model(self):
        pass

    def save_model(self):
        exporter = export.Exporter(self.path)
        exporter["fst"] = self.model
        exporter.close()
