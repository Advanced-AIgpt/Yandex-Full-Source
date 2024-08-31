from alice.nlu.tools.ar_fst_entities.common import utils
from alice.nlu.tools.ar_fst_entities.common.base_fst import BaseFst
from alice.nlu.tools.ar_fst_entities.common.json_keys import JsonKeys
from alice.nlu.tools.ar_fst_entities.common.number_fst import NumberFst
import pynini
from pynini.lib import pynutil


class DateFst(BaseFst):
    def __init__(self, name, dictionaries_path, path):
        super(DateFst, self).__init__(name, dictionaries_path, path)
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
        self.months_fst = utils.fst_from_dict_file(self.dictionaries_path, "date/months.yaml")
        self.special_relative_dates_fst = utils.fst_from_dict_file(
            self.dictionaries_path, "date/special_relative_dates.yaml")
        month_word = utils.load_dict(self.dictionaries_path, "date/month_word.yaml")
        self.delete_month_word_fst = pynutil.delete(pynini.string_map(month_word["singular"] + month_word["plural"]))
        self.optional_delete_month_word_fst = self.delete_month_word_fst.ques
        year_word = utils.load_dict(self.dictionaries_path, "date/year_word.yaml")
        self.delete_year_word_fst = pynutil.delete(pynini.string_map(year_word["singular"] + year_word["plural"]))
        self.optional_delete_year_word_fst = self.delete_year_word_fst.ques
        word_every_fst = utils.fst_from_tsv_file(self.dictionaries_path, "common/word_every.tsv")
        self.delete_word_every_fst = pynutil.delete(word_every_fst)
        weekly_fst = utils.fst_from_tsv_file(self.dictionaries_path, "date/weekly.tsv")
        self.delete_weekly_fst = pynutil.delete(weekly_fst)
        self.monthly_fst = utils.fst_from_tsv_file(self.dictionaries_path, "date/monthly.tsv")
        self.delete_monthly_fst = pynutil.delete(self.monthly_fst)
        self.yearly_fst = utils.fst_from_tsv_file(self.dictionaries_path, "date/yearly.tsv")
        self.delete_yearly_fst = pynutil.delete(self.yearly_fst)
        date_separators_fst = utils.fst_from_tsv_file(self.dictionaries_path, "date/date_separators.tsv")
        self.delete_date_separators = pynutil.delete(date_separators_fst)

    def get_standard_date_fst(self, number_fst):
        day_fst = pynutil.insert(utils.make_json_key(JsonKeys.days), weight=-2) + number_fst + pynutil.insert(",")
        month_fst = pynutil.insert(utils.make_json_key(JsonKeys.months), weight=-2)
        month_fst += (self.common.optional_delete_space_fst +
                      self.common.optional_delete_from_word_fst +
                      self.common.optional_delete_space_fst +
                      self.common.optional_delete_article_fst +
                      ((self.optional_delete_month_word_fst +
                        self.common.optional_delete_space_fst +
                        self.months_fst) | (self.delete_month_word_fst +
                                            self.common.optional_delete_space_fst +
                                            number_fst)) +
                      self.common.optional_delete_space_fst)
        month_fst += pynutil.insert(",")
        year_fst = pynutil.insert(utils.make_json_key(JsonKeys.years), weight=-2)
        year_fst += (self.common.optional_delete_space_fst +
                     self.common.optional_delete_from_word_fst +
                     self.common.optional_delete_space_fst +
                     self.common.optional_delete_article_fst +
                     self.optional_delete_year_word_fst +
                     self.common.optional_delete_space_fst +
                     number_fst +
                     self.common.optional_delete_space_fst +
                     self.common.optional_delete_space_fst)
        year_fst += pynutil.insert(",")
        result_fst = day_fst + self.common.optional_delete_space_fst + self.common.optional_delete_from_word_fst + \
            self.common.optional_delete_space_fst + month_fst + self.common.optional_delete_space_fst + year_fst
        result_fst |= month_fst + self.common.optional_delete_space_fst + year_fst
        result_fst |= day_fst + self.common.optional_delete_space_fst + month_fst
        result_fst |= month_fst | year_fst | day_fst
        return result_fst.optimize()

    def get_date_as_numbers_fst(self, number_fst):
        day_num_fst = pynutil.insert(utils.make_json_key(JsonKeys.days), weight=-2)
        day_num_fst += number_fst + pynutil.insert(",")
        month_num_fst = pynutil.insert(utils.make_json_key(JsonKeys.months), weight=-2)
        month_num_fst += self.delete_date_separators + number_fst + pynutil.insert(",")
        year_num_fst = pynutil.insert(utils.make_json_key(JsonKeys.years), weight=-2)
        year_num_fst += self.delete_date_separators + number_fst + pynutil.insert(",")
        result_fst = day_num_fst | (day_num_fst + month_num_fst) | (day_num_fst + month_num_fst + year_num_fst)
        return result_fst.optimize()

    def get_final_standard_date_fst(self, number_fst):
        standard_date_form_fst = self.get_standard_date_fst(number_fst)
        standard_date_form_fst |= self.get_date_as_numbers_fst(number_fst)
        result_fst = pynutil.insert("{") + pynutil.insert(utils.make_json_key_value(JsonKeys.is_relative, "false"))
        result_fst += self.common.optional_delete_space_fst + standard_date_form_fst + pynutil.insert("}")
        return result_fst

    def get_special_relative_dates_fst(self):
        result_fst = pynutil.insert("{") + pynutil.insert(utils.make_json_key_value(JsonKeys.is_relative, "true"))
        result_fst += pynutil.insert(utils.make_json_key(JsonKeys.days)) + \
            self.special_relative_dates_fst + pynutil.insert("}")
        return result_fst.optimize()

    def get_week_day_with_singular_day_or_from_word_fst(self):
        result_fst = (self.common.article_fst.ques + (
            pynini.string_map(self.day_word["singular"]) | self.common.from_word_fst).ques) @ self.common.replace_with_sharp
        result_fst += pynutil.insert("{") + pynutil.insert(utils.make_json_key(JsonKeys.week_day))
        result_fst += self.common.optional_delete_space_fst + self.week_day_fst + pynutil.insert("}")
        return result_fst.optimize()

    def get_today_fst(self):
        result_fst = pynutil.insert("{") + pynutil.insert(utils.make_json_key_value(JsonKeys.is_relative, "true")) + \
            pynutil.insert(utils.make_json_key_value(JsonKeys.repeat, "false")) + \
            pynutil.insert(utils.make_json_key_value(JsonKeys.days, "0")) + \
            self.common.optional_delete_space_fst + self.common.optional_delete_article_fst + \
            self.delete_day_word_single_fst + pynutil.insert("}")
        return result_fst.optimize()

    def get_every_week_fst(self):
        result_fst = pynutil.insert("{") + pynutil.insert(utils.make_json_key_value(JsonKeys.is_relative, "true")) + \
            pynutil.insert(utils.make_json_key_value(JsonKeys.repeat, "true")) + \
            pynutil.insert(utils.make_json_key_value(JsonKeys.days, "7")) + \
            self.common.optional_delete_space_fst + (self.common.optional_delete_article_fst +
                                                     self.delete_weekly_fst) + pynutil.insert("}")
        return result_fst.optimize()

    def get_yearly_fst(self):
        result_fst = pynutil.insert("{") + self.common.optional_delete_space_fst + self.delete_yearly_fst + pynutil.insert(
            utils.make_json_key_value(JsonKeys.is_relative, "true")) + pynutil.insert(
            utils.make_json_key_value(JsonKeys.repeat, "true")) + pynutil.insert(
            utils.make_json_key(JsonKeys.years) + "1,") + pynutil.insert("}")
        return result_fst.optimize()

    def get_monthly_fst(self):
        result_fst = pynutil.insert("{") + self.common.optional_delete_space_fst + self.delete_monthly_fst + \
            pynutil.insert(utils.make_json_key_value(JsonKeys.is_relative, "true")) + \
            pynutil.insert(utils.make_json_key_value(JsonKeys.repeat, "true")) + \
            pynutil.insert(utils.make_json_key_value(JsonKeys.months, "1")) + pynutil.insert("}")
        return result_fst.optimize()

    def get_general_repeat_fst(self, number_fst):
        result_fst = pynutil.insert("{") + pynutil.insert(
            utils.make_json_key_value(JsonKeys.is_relative, "true"))
        result_fst += pynutil.insert(utils.make_json_key_value(JsonKeys.repeat, "true"))
        result_fst += self.common.optional_delete_space_fst + self.delete_word_every_fst + \
            self.common.optional_delete_space_fst
        years_fst = pynutil.insert(utils.make_json_key(JsonKeys.years))
        years_fst += number_fst + self.common.optional_delete_space_fst + self.common.optional_delete_article_fst + \
            self.delete_year_word_fst
        years_fst += pynutil.insert(",")
        result_fst += years_fst.ques
        result_fst += self.common.optional_delete_space_fst
        months_fst = pynutil.insert(utils.make_json_key(JsonKeys.months))
        months_fst += number_fst + self.common.optional_delete_space_fst + self.common.optional_delete_article_fst + \
            self.delete_month_word_fst
        months_fst += pynutil.insert(",")
        result_fst += months_fst.ques
        result_fst += self.common.optional_delete_space_fst
        days_fst = pynutil.insert(utils.make_json_key(JsonKeys.days))
        days_fst += number_fst + self.common.optional_delete_space_fst + self.common.optional_delete_article_fst + (
            self.delete_day_word_single_fst | self.delete_day_word_plural_fst)
        days_fst += pynutil.insert(",")
        result_fst += days_fst.ques + pynutil.insert("}")
        return result_fst.optimize()

    def get_pure_date(self):
        number_fst = NumberFst.get_static_pure_number(self.dictionaries_path, 6)
        functions_with_params = {
            "standard_date_form_fst": [self.get_final_standard_date_fst, number_fst],
            "special_relative_dates_fst": [self.get_special_relative_dates_fst],
            "day_singular_or_from_word_plus_week_day": [self.get_week_day_with_singular_day_or_from_word_fst],
            "today_fst": [self.get_today_fst],
            "every_week_fst": [self.get_every_week_fst],
            "yearly_fst": [self.get_yearly_fst],
            "monthly_fst": [self.get_monthly_fst],
            "general_repeat_fst": [self.get_general_repeat_fst, number_fst]
        }
        models = utils.parallelize_functions(functions_with_params)
        result_fst = pynini.union(*models.values())
        return result_fst.optimize()

    def create_model(self):
        date_final_fst = self.get_pure_date()
        date_final_fst = pynini.closure(pynutil.add_weight(self.common.alphabet, -1)).optimize() @ date_final_fst
        date_final_fst = self.common.replace_with_sharp + date_final_fst + self.common.replace_with_sharp
        date_final_fst = self.common.delete_extra_space @ self.common.doubled_form_fst @ date_final_fst
        self.model = date_final_fst.optimize()
