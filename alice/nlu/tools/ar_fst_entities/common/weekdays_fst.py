from alice.nlu.tools.ar_fst_entities.common import utils
from alice.nlu.tools.ar_fst_entities.common.base_fst import BaseFst
from alice.nlu.tools.ar_fst_entities.common.json_keys import JsonKeys
import pynini
from pynini.lib import pynutil


class WeekdaysFst(BaseFst):
    def __init__(self, name, dictionaries_path, path):
        super(WeekdaysFst, self).__init__(name, dictionaries_path, path)
        self.day_word = utils.load_dict(self.dictionaries_path, "date/day_word.yaml")
        self.delete_day_word_plural_fst = pynutil.delete(pynini.string_map(self.day_word["plural"]))
        self.optional_delete_plural_day_word_fst = self.delete_day_word_plural_fst.ques
        self.delete_day_word_single_fst = pynutil.delete(pynini.string_map(self.day_word["singular"]))
        self.optional_delete_single_day_word_fst = self.delete_day_word_single_fst.ques
        week_day_fst = utils.fst_from_dict_file(self.dictionaries_path, "date/week_days.yaml")
        self.week_day_fst = pynutil.add_weight(week_day_fst, -7)
        self.week_day_fst = (
            self.common.optional_delete_space_fst +
            self.common.optional_delete_waw_fst +
            self.common.optional_delete_space_fst +
            (self.optional_delete_plural_day_word_fst | self.optional_delete_single_day_word_fst) +
            self.common.optional_delete_space_fst +
            self.common.optional_delete_article_fst +
            self.week_day_fst +
            self.common.optional_delete_space_fst)
        word_except_fst = utils.fst_from_tsv_file(self.dictionaries_path, "date/word_except.tsv")
        self.delete_except_words_fst = pynutil.delete(word_except_fst)
        self.optional_except_days_fst = self.common.optional_delete_space_fst + self.delete_except_words_fst + \
            self.common.optional_delete_space_fst
        self.optional_except_days_fst += pynutil.insert(utils.make_json_key(JsonKeys.except_week_days) + "\\[")
        self.optional_except_days_fst += pynini.closure(self.week_day_fst + pynutil.insert(", "), 1) + \
            pynutil.insert("\\],")
        self.optional_except_days_fst = self.optional_except_days_fst.ques
        word_every_fst = utils.fst_from_tsv_file(self.dictionaries_path, "common/word_every.tsv")
        self.delete_word_every_fst = pynutil.delete(word_every_fst)
        dayly_fst = utils.fst_from_tsv_file(self.dictionaries_path, "date/dayly.tsv")
        self.delete_dayly_fst = pynutil.delete(dayly_fst)
        self.weekend_fst = utils.fst_from_tsv_file(self.dictionaries_path, "date/weekend.tsv")
        self.delete_weekend_fst = pynutil.delete(self.weekend_fst)
        self.working_days_fst = utils.fst_from_tsv_file(self.dictionaries_path, "date/working_days.tsv")
        self.delete_working_days_fst = pynutil.delete(self.working_days_fst)

    def get_week_day_with_singular_day_or_from_word_fst(self):
        result_fst = (self.common.article_fst.ques + (
            pynini.string_map(self.day_word["singular"]) | self.common.from_word_fst).ques) @ self.common.replace_with_sharp
        result_fst += pynutil.insert("{") + pynutil.insert(utils.make_json_key(JsonKeys.week_days) + "\\[")
        result_fst += self.common.optional_delete_space_fst + \
            pynini.closure(self.week_day_fst + pynutil.insert(", "), 1) + pynutil.insert("\\],")
        result_fst += ((self.common.optional_delete_space_fst + self.common.repeat_fst) | pynutil.insert(
            utils.make_json_key_value(JsonKeys.repeat, "false"))) + pynutil.insert("}")
        return result_fst.optimize()

    def get_week_day_with_plural_day_word_fst(self):
        result_fst = (pynini.closure(self.common.white_space) + self.common.article_fst.ques +
                      pynini.string_map(self.day_word["plural"]) + pynini.closure(self.common.white_space)) @ \
            self.common.replace_with_sharp
        result_fst += pynutil.insert("{") + pynutil.insert(utils.make_json_key_value(JsonKeys.repeat, "true")) + \
            pynutil.insert(utils.make_json_key(JsonKeys.week_days) + "\\[")
        result_fst += pynini.closure(self.week_day_fst, 1, 1) + pynutil.insert("\\],") + pynutil.insert("}")
        return result_fst.optimize()

    def get_multi_week_days_with_plural_day_word_fst(self):
        result_fst = (pynini.closure(self.common.white_space) + self.common.article_fst.ques + pynini.string_map(
            self.day_word["plural"]) + pynini.closure(self.common.white_space)) @ self.common.replace_with_sharp
        result_fst += pynutil.insert("{") + pynutil.insert(utils.make_json_key(JsonKeys.week_days) + "\\[")
        result_fst += pynini.closure(self.week_day_fst + pynutil.insert(", "), 2) + pynutil.insert("\\],")
        result_fst += ((self.common.optional_delete_space_fst + self.common.repeat_fst) | pynutil.insert(
            utils.make_json_key_value(JsonKeys.repeat, "false"))) + pynutil.insert("}")
        return result_fst.optimize()

    def get_everyday_fst(self):
        result_fst = pynutil.insert("{") + pynutil.insert(utils.make_json_key_value(JsonKeys.repeat, "true"))
        result_fst += pynutil.insert(utils.make_json_key_value(JsonKeys.week_days, "\\[0, 1, 2, 3, 4, 5, 6\\]")) + \
            self.common.optional_delete_space_fst
        result_fst += ((self.common.optional_delete_article_fst +
                        self.delete_dayly_fst) | (self.delete_word_every_fst +
                                                  self.common.optional_delete_space_fst +
                                                  pynutil.delete(self.common.day_part_fst)))
        result_fst += self.common.optional_delete_space_fst + self.optional_except_days_fst + pynutil.insert("}")
        return result_fst.optimize()

    def get_every_with_week_day_fst(self):
        result_fst = pynutil.insert("{") + pynutil.insert(utils.make_json_key_value(JsonKeys.repeat, "true"))
        result_fst += pynutil.insert(utils.make_json_key(JsonKeys.week_days) + "\\[")
        result_fst += self.common.optional_delete_space_fst + self.delete_word_every_fst + \
            self.common.optional_delete_space_fst
        result_fst += (pynutil.delete(self.common.day_part_fst) + self.common.optional_delete_space_fst).ques
        result_fst += (
            self.optional_delete_single_day_word_fst | self.optional_delete_plural_day_word_fst) + \
            self.common.optional_delete_space_fst
        result_fst += pynini.closure(self.week_day_fst + pynutil.insert(", "), 1) + pynutil.insert("\\],") + \
            pynutil.insert("}")
        return result_fst.optimize()

    def get_weekend_fst(self):
        result_fst = pynutil.insert("{") + pynutil.insert(utils.make_json_key_value(JsonKeys.week_days, "\\[4, 5\\]"))
        result_fst += self.common.optional_delete_space_fst + self.common.optional_delete_article_fst + \
            self.delete_weekend_fst
        result_fst += (self.common.optional_delete_space_fst + self.common.repeat_fst) | \
            pynutil.insert(utils.make_json_key_value(JsonKeys.repeat, "false"))
        result_fst += self.common.optional_delete_space_fst + self.optional_except_days_fst + pynutil.insert("}")
        return result_fst.optimize()

    def get_working_days_fst(self):
        result_fst = pynutil.insert("{") + pynutil.insert(
            utils.make_json_key_value(JsonKeys.week_days, "\\[0, 1, 2, 3, 6\\]"))
        result_fst += self.common.optional_delete_space_fst + self.common.optional_delete_article_fst + \
            self.delete_working_days_fst
        result_fst += ((self.common.optional_delete_space_fst + self.common.repeat_fst) | pynutil.insert(
            utils.make_json_key_value(JsonKeys.repeat, "false"))) + self.common.optional_delete_space_fst + \
            self.optional_except_days_fst + pynutil.insert("}")
        return result_fst.optimize()

    def get_week_days_range_fst(self):
        result_fst = pynutil.insert("{") + self.common.optional_delete_space_fst + self.common.delete_from_word_fst + \
            self.common.optional_delete_space_fst + self.optional_delete_single_day_word_fst + \
            self.common.optional_delete_space_fst
        result_fst += pynutil.insert(utils.make_json_key(JsonKeys.week_days_start))
        result_fst += self.common.optional_delete_article_fst + pynini.closure(self.week_day_fst, 1, 1)
        result_fst += pynutil.insert(",")
        result_fst += self.common.optional_delete_space_fst + self.common.delete_to_word_fst + \
            self.common.optional_delete_space_fst + self.optional_delete_single_day_word_fst + \
            self.common.optional_delete_space_fst
        result_fst += pynutil.insert(utils.make_json_key(JsonKeys.week_days_end))
        result_fst += self.common.optional_delete_article_fst + pynini.closure(self.week_day_fst, 1, 1)
        result_fst += pynutil.insert(",") + self.common.optional_delete_space_fst
        result_fst += ((self.common.optional_delete_space_fst + self.common.repeat_fst) | pynutil.insert(
            utils.make_json_key_value(JsonKeys.repeat, "false")))
        result_fst += self.optional_except_days_fst + pynutil.insert("}")
        return result_fst.optimize()

    def get_pure_weekdays(self):
        functions_with_params = {
            "day_singular_or_from_word_plus_week_day": [self.get_week_day_with_singular_day_or_from_word_fst],
            "day_plural_plus_one_week_day": [self.get_week_day_with_plural_day_word_fst],
            "day_plural_plus_multi_week_day": [self.get_multi_week_days_with_plural_day_word_fst],
            "everyday_fst": [self.get_everyday_fst],
            "every_plus_week_day_fst": [self.get_every_with_week_day_fst],
            "weekend_fst": [self.get_weekend_fst],
            "working_days_fst": [self.get_working_days_fst],
            "week_days_range_fst": [self.get_week_days_range_fst]
        }
        models = utils.parallelize_functions(functions_with_params)
        result_fst = pynini.union(*models.values())
        return result_fst.optimize()

    def create_model(self):
        weekdays_final_fst = self.get_pure_weekdays()
        weekdays_final_fst = pynini.closure(pynutil.add_weight(self.common.alphabet, -1)).optimize() @ weekdays_final_fst
        weekdays_final_fst = self.common.replace_with_sharp + weekdays_final_fst + self.common.replace_with_sharp
        weekdays_final_fst = self.common.delete_extra_space @ self.common.doubled_form_fst @ weekdays_final_fst
        self.model = weekdays_final_fst.optimize()
