from alice.nlu.tools.ar_fst_entities.common import utils
from alice.nlu.tools.ar_fst_entities.common.base_fst import BaseFst
from alice.nlu.tools.ar_fst_entities.common.json_keys import JsonKeys
from functools import cache
import pynini
from pynini.lib import byte, pynutil


class NumberFst(BaseFst):
    def __init__(self, name, dictionaries_path, path):
        super(NumberFst, self).__init__(name, dictionaries_path, path)

    def create_model(self):
        number_fst = NumberFst.get_static_pure_number(self.dictionaries_path)
        number_fst = pynini.closure(pynutil.add_weight(self.common.alphabet, -1)).optimize() @ number_fst
        number_fst = self.common.replace_with_sharp + pynutil.insert("{") + \
            pynutil.insert(utils.make_json_key(JsonKeys.number)) + number_fst + pynutil.insert("}") + \
            self.common.replace_with_sharp
        number_fst = self.common.delete_extra_space @ self.common.doubled_form_fst @ number_fst
        self.model = number_fst.optimize()

    @staticmethod
    @cache
    def get_static_pure_number(dictionaries_path, max_num_digits=None):
        common = utils.CommonFsts(dictionaries_path)
        pre_negative_sign_fst = utils.fst_from_tsv_file(dictionaries_path, "number/pre_negative_sign.tsv")
        pre_negative_sign_fst = common.optional_delete_space_fst + \
            pynutil.delete(pre_negative_sign_fst) + common.optional_delete_space_fst
        pre_negative_sign_fst = pre_negative_sign_fst.optimize()
        post_negative_sign_fst = utils.fst_from_tsv_file(dictionaries_path, "number/post_negative_sign.tsv")
        post_negative_sign_fst = common.optional_delete_space_fst + \
            pynutil.delete(post_negative_sign_fst) + common.optional_delete_space_fst
        post_negative_sign_fst = post_negative_sign_fst.optimize()
        digit_words = utils.load_dict(dictionaries_path, "number/digits.yaml")
        digits_fst = common.optional_delete_article_fst + pynutil.add_weight(utils.fst_from_dict(digit_words), -1)
        digits_fst = digits_fst.optimize()
        tens_fst = utils.fst_from_dict_file(dictionaries_path, "number/tens.yaml")
        tens_fst = common.optional_delete_article_fst + pynutil.add_weight(tens_fst, -1)
        numbers_11_99_fst = digits_fst + common.optional_delete_space_fst + common.optional_delete_waw_fst + \
            common.optional_delete_space_fst + tens_fst
        tens_fst = tens_fst.optimize()
        numbers_11_99_fst |= pynutil.insert("0") + tens_fst
        reverse_two_digit_fst = utils.fst_from_tsv_file(dictionaries_path, "number/reverse_two_digit_numbers.tsv")
        reverse_two_digit_fst = reverse_two_digit_fst
        numbers_11_99_fst = (numbers_11_99_fst @ reverse_two_digit_fst).optimize()
        hundred_fst = utils.fst_from_dict_file(dictionaries_path, "number/hundreds.yaml")
        hundred_fst = common.optional_delete_article_fst + pynutil.add_weight(hundred_fst, -0.1)
        hundred_fst = hundred_fst.optimize()
        multi_hundred_fst = hundred_fst | (
            digits_fst +
            common.optional_delete_space_fst +
            pynutil.delete(hundred_fst))
        digits_not_words_fst = pynini.string_map(list("0123456789"))
        pure_digits = pynutil.add_weight(digits_not_words_fst, -0.1).closure(1)
        arabic_digits_to_normal_digits_fst = utils.fst_from_tsv_file(
            dictionaries_path, "number/arabic_digits_to_normal_digits.tsv")
        pure_digits |= pynutil.add_weight(arabic_digits_to_normal_digits_fst, -0.1).closure(1)
        pure_digits = pure_digits.optimize()
        multi_hundred_fst |= pure_digits + common.optional_delete_space_fst + pynutil.delete(hundred_fst)
        multi_hundred_fst = multi_hundred_fst.optimize()
        hundred_final_fst = multi_hundred_fst + pynutil.insert("00")
        hundred_final_fst |= multi_hundred_fst + common.optional_delete_space_fst + common.delete_waw_fst + \
            numbers_11_99_fst
        hundred_final_fst |= multi_hundred_fst + pynutil.insert("0") + common.optional_delete_space_fst + \
            common.delete_waw_fst + digits_fst
        hundred_final_fst = hundred_final_fst | (pynutil.insert("0") + numbers_11_99_fst)
        hundred_final_fst |= (pynutil.insert("00") + digits_fst) | pure_digits | (pynutil.insert("0") + pure_digits)
        hundred_final_fst |= (pynutil.insert("00") + pure_digits) | pynutil.insert("000", weight=0.1)
        hundred_final_fst = hundred_final_fst.optimize()
        number_fst = pynini.accep("")
        powers_of_ten_words = utils.load_dict(dictionaries_path, "number/powers_of_ten.yaml")
        at_least_one_non_zero_digit = pynutil.delete(pynini.closure("0"))
        at_least_one_non_zero_digit += pynini.difference(byte.DIGIT, "0") + pynini.closure(byte.DIGIT)
        at_least_one_non_zero_digit = at_least_one_non_zero_digit.optimize()
        at_least_one_non_zero_digit_no_delete = pynini.closure("0") + pynini.difference(byte.DIGIT, "0") +\
            pynini.closure(byte.DIGIT)
        at_least_one_non_zero_digit_no_delete = at_least_one_non_zero_digit_no_delete.optimize()
        num_digits = 3
        for pw_key, pw_values in sorted(powers_of_ten_words.items(), key=lambda x: x[0], reverse=True):
            if max_num_digits is not None and pw_key + 3 > max_num_digits:
                continue
            pw_acc = pynini.string_map(pw_values)
            delete_pw_word_fst = pynutil.delete(pw_acc)
            pw_final_fst = (
                hundred_final_fst + common.delete_space_fst) @ \
                at_least_one_non_zero_digit_no_delete + delete_pw_word_fst
            pw_final_fst |= (pynutil.insert("001") + delete_pw_word_fst) | pynutil.insert("000", weight=0.1)
            number_fst += (pw_final_fst + common.optional_delete_space_fst +
                           common.optional_delete_waw_fst).optimize()
            num_digits += 3
        number_fst += hundred_final_fst
        zero = common.optional_delete_article_fst + pynini.cross(pynini.string_map(digit_words[0]), "0")
        zero = zero.optimize()
        number_fst |= zero
        number_fst @= pynini.union(at_least_one_non_zero_digit, "0").optimize()
        all_zeros_to_zero = pynini.cross(pynini.closure("0", 1), "0").optimize()
        number_fst |= pure_digits @ pynini.union(all_zeros_to_zero, at_least_one_non_zero_digit, "0").optimize()
        number_fst |= pre_negative_sign_fst + pynutil.insert("-") + number_fst
        number_fst |= pynutil.insert("-") + number_fst + post_negative_sign_fst
        return number_fst.optimize()
