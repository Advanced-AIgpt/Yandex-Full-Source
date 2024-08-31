from alice.nlu.tools.ar_fst_entities.common import utils
from alice.nlu.tools.ar_fst_entities.common.base_fst import BaseFst
from alice.nlu.tools.ar_fst_entities.common.json_keys import JsonKeys
import pynini
from pynini.lib import pynutil


class SelectionFst(BaseFst):
    def __init__(self, name, dictionaries_path, path):
        super(SelectionFst, self).__init__(name, dictionaries_path, path)

    def create_model(self):
        selection_fst = utils.fst_from_dict_file(self.dictionaries_path, "selection/selection_words.yaml")
        selection_fst = pynutil.insert("{") + pynutil.insert(utils.make_json_key(JsonKeys.selection)) + \
            pynutil.insert("\"") + selection_fst + pynutil.insert("\"") + pynutil.insert("}")
        space_to_sharp = utils.anything_to_sharp(pynini.closure(self.common.white_space, 1), self.dictionaries_path)
        selection_fst = (self.common.replace_with_sharp + space_to_sharp + selection_fst) | (
            selection_fst + space_to_sharp + self.common.replace_with_sharp) | (
            self.common.replace_with_sharp + space_to_sharp + selection_fst + space_to_sharp +
            self.common.replace_with_sharp)
        self.model = selection_fst.optimise()
