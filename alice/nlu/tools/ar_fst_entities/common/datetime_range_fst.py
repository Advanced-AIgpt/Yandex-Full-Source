from alice.nlu.tools.ar_fst_entities.common import utils
from alice.nlu.tools.ar_fst_entities.common.base_fst import BaseFst
from alice.nlu.tools.ar_fst_entities.common.date_fst import DateFst
from alice.nlu.tools.ar_fst_entities.common.datetime_fst import DatetimeFst
from alice.nlu.tools.ar_fst_entities.common.json_keys import JsonKeys
from alice.nlu.tools.ar_fst_entities.common.number_fst import NumberFst
from alice.nlu.tools.ar_fst_entities.common.time_fst import TimeFst
import pynini
from pynini.lib import pynutil


class DatetimeRangeFst(BaseFst):
    def __init__(self, name, dictionaries_path, path):
        super(DatetimeRangeFst, self).__init__(name, dictionaries_path, path)
        self.interval_prefixes = utils.load_dict(dictionaries_path, "datetime_range/interval_prefixes.yaml")
        self.interval_prefixes = utils.union_per_key(self.interval_prefixes)
        self.interval_suffixes = utils.load_dict(dictionaries_path, "datetime_range/interval_suffixes.yaml")
        self.interval_suffixes = utils.union_per_key(self.interval_suffixes)
        day_word = utils.load_dict(self.dictionaries_path, "date/day_word.yaml")
        self.delete_day_word_single_fst = pynutil.delete(pynini.string_map(day_word["singular"]))
        week_word = utils.load_dict(self.dictionaries_path, "date/week_word.yaml")
        self.delete_week_word_single_fst = pynutil.delete(pynini.string_map(week_word["singular"]))
        month_word = utils.load_dict(self.dictionaries_path, "date/month_word.yaml")
        self.delete_month_word_single_fst = pynutil.delete(pynini.string_map(month_word["singular"]))
        year_word = utils.load_dict(self.dictionaries_path, "date/year_word.yaml")
        self.delete_year_word_single_fst = pynutil.delete(pynini.string_map(year_word["singular"]))
        weekend_fst = utils.fst_from_tsv_file(self.dictionaries_path, "date/weekend.tsv")
        self.delete_weekend_fst = pynutil.delete(weekend_fst)

    def get_interval(self, start_int, special_word):
        string_form = "\"{}\":{{\"date\":{{{}{}{}}}}}"
        weekend_json = ""
        if special_word == "weekend":
            weekend_json = utils.make_json_key_value("weekend", "true")
            special_word = "weeks"
        start_value = utils.make_json_key_value(special_word, start_int)
        end_value = utils.make_json_key_value(special_word, start_int + 1)
        is_relative = utils.make_json_key_value(special_word + "_relative", "true")
        start = string_form.format("start", start_value, is_relative, weekend_json)
        end = string_form.format("end", end_value, is_relative, weekend_json)
        return pynutil.insert(start + "," + end)

    def get_prefix(self, special_word, special_fst, start_int, prefix):
        prefix_fst = pynutil.delete(prefix) + self.common.optional_delete_space_fst + \
            self.common.optional_delete_article_fst + special_fst
        prefix_fst += self.get_interval(start_int, special_word)
        return prefix_fst

    def get_suffix(self, special_word, special_fst, start_int, suffix):
        suffix_fst = self.common.optional_delete_article_fst + special_fst + self.common.optional_delete_space_fst + \
            self.common.optional_delete_article_fst + pynutil.delete(suffix)
        suffix_fst += self.get_interval(start_int, special_word)
        return suffix_fst

    def get_special_intervals(self):
        special_words_fsts = {
            "days": self.delete_day_word_single_fst,
            "weeks": self.delete_week_word_single_fst,
            "months": self.delete_month_word_single_fst,
            "years": self.delete_year_word_single_fst,
            "weekend": self.delete_weekend_fst
        }
        fsts = []
        for special_word, special_fst in special_words_fsts.items():
            fsts.append(special_fst + self.get_interval(0, special_word))
            for start_int, prefix in self.interval_prefixes.items():
                fsts.append(self.get_prefix(special_word, special_fst, start_int, prefix))
            for start_int, suffix in self.interval_suffixes.items():
                fsts.append(self.get_suffix(special_word, special_fst, start_int, suffix))
        return pynini.union(*fsts)

    def get_date_fst(self, number_fst):
        date_object = DateFst("date", self.dictionaries_path, None)
        functions_with_params = {
            "standard_date_form_fst": [date_object.get_final_standard_date_fst, number_fst],
            "day_singular_or_from_word_plus_week_day": [date_object.get_week_day_with_singular_day_or_from_word_fst],
            "today_fst": [date_object.get_today_fst],
        }
        models = utils.parallelize_functions(functions_with_params)
        result_fst = pynini.union(*models.values())
        return result_fst

    def get_time_fst(self, number_fst):
        time_object = TimeFst("time", self.dictionaries_path, None)
        functions_with_params = {
            "absolute_time_fst": [time_object.get_absolute_time_fst, number_fst],
            "absolute_time_to_fst": [time_object.get_absolute_time_to_fst, number_fst],
        }
        models = utils.parallelize_functions(functions_with_params)
        result_fst = pynini.union(*models.values())
        return result_fst

    def get_datetime_interval(self):
        number_fst = NumberFst.get_static_pure_number(self.dictionaries_path, 6)
        functions_with_params = {
            "date_fst": [self.get_date_fst, number_fst],
            "time_fst": [self.get_time_fst, number_fst]
        }
        models = utils.parallelize_functions(functions_with_params)
        datetime_object = DatetimeFst("datetime", self.dictionaries_path, None)
        separator_fst = datetime_object.get_separators()
        date_fst = models["date_fst"] @ self.common.remove_sharp
        time_fst = models["time_fst"] @ self.common.remove_sharp
        date_fst = pynutil.insert(utils.make_json_key(JsonKeys.date)) + date_fst + pynutil.insert(",")
        time_fst = pynutil.insert(utils.make_json_key(JsonKeys.time)) + time_fst + pynutil.insert(",")
        functions_with_params = {
            "optimized_date_time": [datetime_object.get_optimized_date_time, date_fst, separator_fst, time_fst],
            "optimized_time_date": [datetime_object.get_optimized_time_date, time_fst, separator_fst, date_fst],
            "optimized_date": [datetime_object.get_optimized_date, date_fst],
            "optimized_time": [datetime_object.get_optimized_time, time_fst]
        }
        models = utils.parallelize_functions(functions_with_params)
        datetime_fst = pynutil.insert("{") + pynini.union(*models.values()) + pynutil.insert("}")
        time_triggers = datetime_object.get_triggers("time/time_trigger_words.yaml")
        date_triggers = datetime_object.get_triggers("date/date_trigger_words.yaml")
        delete_triggers_fst = self.common.optional_delete_space_fst + self.common.optional_delete_article_fst + \
            pynutil.delete(time_triggers | date_triggers).closure() + self.common.optional_delete_space_fst
        delete_triggers_fst = delete_triggers_fst.optimize()
        datetime_interval = (self.common.optional_delete_from_word_fst + delete_triggers_fst +
                             utils.make_json_key_fst_value(JsonKeys.datetime_range_start, datetime_fst) +
                             self.common.optional_delete_space_fst).ques
        datetime_interval += self.common.optional_delete_waw_fst + self.common.optional_delete_space_fst + \
            self.common.delete_to_word_fst + delete_triggers_fst + \
            utils.make_json_key_fst_value(JsonKeys.datetime_range_end, datetime_fst)
        datetime_interval |= self.common.delete_from_word_fst + delete_triggers_fst + \
            utils.make_json_key_fst_value(JsonKeys.datetime_range_start, datetime_fst)
        return datetime_interval

    def get_pure_datetime_range(self):
        functions_with_params = {
            "datetime_interval_fst": [self.get_datetime_interval],
            "special_intervals_fst": [self.get_special_intervals]
        }
        models = utils.parallelize_functions(functions_with_params)
        result_fst = pynini.union(*models.values())
        return result_fst

    def create_model(self):
        datetime_range_fst = self.get_pure_datetime_range()
        datetime_range_fst = pynini.closure(pynutil.add_weight(self.common.alphabet, -1)).optimize() @ datetime_range_fst
        datetime_range_fst = self.common.replace_with_sharp + \
            pynutil.insert("{") + datetime_range_fst + pynutil.insert("}") + self.common.replace_with_sharp
        datetime_range_fst = self.common.delete_extra_space @ self.common.doubled_form_fst @ datetime_range_fst
        self.model = datetime_range_fst.optimize()
