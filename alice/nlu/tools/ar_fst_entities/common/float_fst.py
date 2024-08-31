from alice.nlu.tools.ar_fst_entities.common import utils
from alice.nlu.tools.ar_fst_entities.common.base_fst import BaseFst
from alice.nlu.tools.ar_fst_entities.common.json_keys import JsonKeys
from alice.nlu.tools.ar_fst_entities.common.number_fst import NumberFst
import pynini
from pynini.lib import pynutil


class FloatFst(BaseFst):
    def __init__(self, name, dictionaries_path, path):
        super(FloatFst, self).__init__(name, dictionaries_path, path)
        comma_fst = utils.fst_from_tsv_file(self.dictionaries_path, "number/comma.tsv")
        self.delete_comma_fst = pynutil.delete(comma_fst).optimize()
        ones_fst = utils.fst_from_tsv_file(self.dictionaries_path, "number/ones.tsv")
        delete_ones_fst = pynutil.delete(ones_fst)
        self.optional_delete_ones_fst = delete_ones_fst.ques.optimize()
        fraction_fst = utils.fst_from_tsv_file(self.dictionaries_path, "number/fraction.tsv")
        delete_fraction_fst = pynutil.delete(fraction_fst)
        self.optional_delete_fraction_fst = delete_fraction_fst.ques.optimize()
        in_fst = utils.fst_from_tsv_file(self.dictionaries_path, "number/in.tsv")
        self.delete_in_fst = pynutil.delete(in_fst).optimize()
        ten_and_hundred_fractions_fst = utils.fst_from_dict_file(
            self.dictionaries_path, "number/ten_and_hundred_fractions.yaml")
        powers_of_ten_fst = utils.fst_from_dict_file(self.dictionaries_path, "number/powers_of_ten.yaml")
        self.ten_power_fractions_fst = pynini.union(ten_and_hundred_fractions_fst, powers_of_ten_fst).optimize()
        self.special_fractions_fst = utils.fst_from_dict_file(self.dictionaries_path, "number/special_fractions.yaml").optimize()
        self.special_fractions_doubled_fst = utils.fst_from_dict_file(
            self.dictionaries_path, "number/special_fractions_doubled.yaml").optimize()

    def get_pure_float(self, number_fst):
        number = (
            pynutil.insert(utils.make_json_key(JsonKeys.number)) +
            number_fst +
            pynutil.insert(",") +
            self.common.optional_delete_space_fst +
            self.optional_delete_ones_fst +
            self.common.optional_delete_space_fst +
            self.common.optional_delete_waw_fst +
            self.common.optional_delete_space_fst)
        comma = self.delete_comma_fst + self.common.optional_delete_space_fst
        multiplier = (
            pynutil.insert(utils.make_json_key(JsonKeys.multiplier)) +
            number_fst +
            pynutil.insert(",") +
            self.common.optional_delete_space_fst +
            self.optional_delete_fraction_fst +
            self.common.optional_delete_space_fst)
        in_ = self.delete_in_fst + self.common.optional_delete_space_fst
        number_divisor = (
            (pynutil.insert(utils.make_json_key(JsonKeys.number_divisor)) + number_fst +
             pynutil.insert(",") + self.common.optional_delete_space_fst).ques)
        power_divisor = (
            pynutil.insert(utils.make_json_key(JsonKeys.power_divisor)) + self.common.optional_delete_article_fst +
            self.ten_power_fractions_fst + pynutil.insert(","))
        fraction = number.ques + multiplier + in_ + number_divisor + power_divisor
        fraction_with_comma = (
            comma + multiplier + ((in_ + number_divisor + power_divisor) |
                                  pynutil.insert(utils.make_json_key_value(JsonKeys.power_divisor, -1))))
        fraction_with_comma = number + fraction_with_comma
        special_fraction = (
            pynutil.insert(utils.make_json_key(JsonKeys.multiplier)) + number_fst + pynutil.insert(",") +
            self.common.optional_delete_space_fst).ques
        special_fraction = (
            pynutil.add_weight(special_fraction, 1) +
            pynutil.insert(utils.make_json_key(JsonKeys.number_divisor)) +
            self.special_fractions_fst +
            pynutil.insert(","))
        special_fraction = number.ques + special_fraction
        special_fraction_doubled = (
            pynutil.insert(utils.make_json_key_value(JsonKeys.multiplier, 2)) +
            pynutil.insert(utils.make_json_key(JsonKeys.number_divisor)) +
            self.special_fractions_doubled_fst +
            pynutil.insert(","))
        special_fraction_doubled = number.ques + special_fraction_doubled
        return fraction | fraction_with_comma | special_fraction | special_fraction_doubled

    def create_model(self):
        number_fst = NumberFst.get_static_pure_number(self.dictionaries_path)
        pure_float = self.get_pure_float(number_fst)
        integer = pynutil.insert(utils.make_json_key(JsonKeys.number)) + number_fst
        float_fst = integer | pure_float
        float_fst = pynini.closure(pynutil.add_weight(self.common.alphabet, -1)).optimize() @ float_fst
        float_fst = pynutil.insert("{") + float_fst + pynutil.insert("}")
        float_fst = self.common.replace_with_sharp + float_fst + self.common.replace_with_sharp
        float_fst = self.common.delete_extra_space @ float_fst
        self.model = float_fst.optimize()
