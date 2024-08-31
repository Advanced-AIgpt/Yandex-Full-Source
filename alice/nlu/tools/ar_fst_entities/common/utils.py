import concurrent.futures
import os
import pynini
import yaml
from pynini.lib import pynutil, rewrite
from alice.nlu.tools.ar_fst_entities.common.json_keys import JsonKeys


UNICODE_WHITESPACE_CHARACTERS = [
    "\u0009",  # character tabulation
    "\u000a",  # line feed
    "\u000b",  # line tabulation
    "\u000c",  # form feed
    "\u000d",  # carriage return
    "\u0020",  # space
    "\u0085",  # next line
    "\u00a0",  # no-break space
    "\u1680",  # ogham space mark
    "\u2000",  # en quad
    "\u2001",  # em quad
    "\u2002",  # en space
    "\u2003",  # em space
    "\u2004",  # three-per-em space
    "\u2005",  # four-per-em space
    "\u2006",  # six-per-em space
    "\u2007",  # figure space
    "\u2008",  # punctuation space
    "\u2009",  # thin space
    "\u200A",  # hair space
    "\u2028",  # line separator
    "\u2029",  # paragraph separator
    "\u202f",  # narrow no-break space
    "\u205f",  # medium mathematical space
    "\u3000",  # ideographic space
]


class CommonFsts:
    def __init__(self, dictionaries_path):
        single_byte = _byte_range(0x01, 0x7F)
        leading_byte_2_byte = _byte_range(0xD8, 0xD9)
        contiuation_byte = _byte_range(0x80, 0xBF)
        self.alphabet = pynini.union(single_byte, leading_byte_2_byte + contiuation_byte).optimize()

        self.alphabet_acc = pynini.closure(self.alphabet).optimize()
        self.replace_with_sharp = pynutil.add_weight(pynini.cross(self.alphabet, "#"), 1).closure().optimize()
        self.white_space = pynini.union(*UNICODE_WHITESPACE_CHARACTERS).optimize()
        self.delete_extra_space = pynini.cross(pynini.closure(self.white_space, 1), " ")
        self.delete_extra_space = pynini.cdrewrite(self.delete_extra_space, "", "", self.alphabet_acc).optimize()
        self.delete_space_fst = pynutil.delete(pynini.closure(self.white_space)).optimize()
        self.optional_delete_space_fst = self.delete_space_fst.ques
        self.and_fst = fst_from_tsv_file(dictionaries_path, "common/and_word.tsv")
        self.delete_waw_fst = (pynutil.delete(self.and_fst) + self.optional_delete_space_fst).optimize()
        self.optional_delete_waw_fst = self.delete_waw_fst.ques
        self.article_fst = fst_from_tsv_file(dictionaries_path, "common/article.tsv")
        self.delete_article_fst = pynutil.delete(self.article_fst)
        self.optional_delete_article_fst = self.delete_article_fst.ques.optimize()
        self.doubled_form_fst = fst_from_dict_file(dictionaries_path, "common/doubled_form.yaml")
        self.doubled_form_fst = pynini.cdrewrite(self.doubled_form_fst, "", "", self.alphabet_acc).optimize()
        self.day_part_fst = self.get_day_part_fst(dictionaries_path).optimize()
        self.from_word_fst = fst_from_tsv_file(dictionaries_path, "common/from_word.tsv")
        self.delete_from_word_fst = pynutil.delete(self.from_word_fst).optimize()
        self.optional_delete_from_word_fst = self.delete_from_word_fst.ques
        to_word_fst = fst_from_tsv_file(dictionaries_path, "common/to_word.tsv")
        self.delete_to_word_fst = pynutil.delete(to_word_fst).optimize()
        self.repeat_fst = fst_from_tsv_file(dictionaries_path, "common/repeat_words.tsv")
        self.repeat_fst = (pynutil.insert(make_json_key_value(JsonKeys.repeat, "true")) +
                           pynutil.delete(self.repeat_fst)).optimize()
        self.remove_sharp = pynini.cdrewrite(pynini.cross("#", ""), "", "", self.alphabet_acc)

    def get_day_part_fst(self, dictionaries_path):
        day_part_fst = fst_from_dict_file(dictionaries_path, "common/day_part_words.yaml")
        delete_pre_day_part_words = pynutil.delete(fst_from_tsv_file(
            dictionaries_path, "common/pre_day_part_words.tsv"))
        result_fst = pynutil.insert(make_json_key(JsonKeys.day_part), weight=-1)
        result_fst += delete_pre_day_part_words.ques + self.optional_delete_space_fst + \
            self.optional_delete_article_fst + pynutil.insert("\"") + day_part_fst + pynutil.insert("\",")
        return result_fst


def _byte_range(min_val, max_val=None) -> pynini.Fst:
    if max_val is None:
        max_val = min_val
    return pynini.union(*(f"[{i}]" for i in range(min_val, max_val + 1)))


def make_json_key(key):
    return "\"{}\":".format(key)


def make_json_key_value(key, value):
    return "\"{}\":{},".format(key, value)


def make_json_key_fst_value(key, fst):
    return pynutil.insert(make_json_key(key)) + fst + pynutil.insert(",")


def load_yaml(path):
    with open(path, "r") as file:
        return yaml.safe_load(file)


def fst_from_dict(dict_):
    fsts = []
    for key, values in dict_.items():
        for v in values:
            fsts.append(pynini.cross(str(v), str(key)))
    result_fst = pynini.union(*fsts)
    return result_fst


def union_per_key(dict_):
    for key, value in dict_.items():
        dict_[key] = pynini.string_map(value)
    return dict_


def anything_to_sharp(input_fst, dictionaries_path):
    return input_fst @ CommonFsts(dictionaries_path).replace_with_sharp


def load_fst(path):
    return pynini.Far(path, mode="r", arc_type="standard", far_type="default").get_fst()


def get_top_matches(text, fst):
    try:
        return rewrite.top_rewrites(text, fst, nshortest=100)
    except rewrite.Error:
        return []


def load_dict(dictionaries_path, entity_file):
    return load_yaml(os.path.join(dictionaries_path, entity_file))


def fst_from_tsv_file(dictionaries_path, entity_file):
    return pynini.string_file(os.path.join(dictionaries_path, entity_file))


def fst_from_dict_file(dictionaries_path, entity_file):
    return fst_from_dict(load_dict(dictionaries_path, entity_file))


def parallelize_functions(functions_with_params):
    with concurrent.futures.ProcessPoolExecutor() as executor:
        models = [executor.submit(*functon_and_params) for functon_and_params in functions_with_params.values()]
        concurrent.futures.wait(models)
    result_models = dict()
    for idx, key in enumerate(functions_with_params.keys()):
        result_models[key] = models[idx].result()
    return result_models
