from alice.nlu.tools.ar_fst_entities.common import utils
from alice.nlu.tools.ar_fst_entities.common.base_fst import BaseFst
from alice.nlu.tools.ar_fst_entities.common.date_fst import DateFst
from alice.nlu.tools.ar_fst_entities.common.json_keys import JsonKeys
from alice.nlu.tools.ar_fst_entities.common.time_fst import TimeFst
import pynini
from pynini.lib import pynutil


class DatetimeFst(BaseFst):
    def __init__(self, name, dictionaries_path, path):
        super(DatetimeFst, self).__init__(name, dictionaries_path, path)

    def get_triggers(self, path):
        trigger_words = utils.load_dict(self.dictionaries_path, path)
        trigger_fsts = []
        for value in trigger_words.values():
            trigger_fsts.append(pynini.string_map(value))
        trigger_fsts = pynini.union(*trigger_fsts)
        return trigger_fsts

    def get_optimized_date_time(self, date_fst, separator_fst, time_fst):
        return pynutil.add_weight(date_fst + separator_fst + time_fst, 7).optimize()

    def get_optimized_time_date(self, time_fst, separator_fst, date_fst):
        return pynutil.add_weight(time_fst + separator_fst + date_fst, 7).optimize()

    def get_optimized_date(self, date_fst):
        return date_fst.optimize()

    def get_optimized_time(self, time_fst):
        return time_fst.optimize()

    def get_separators(self):
        time_triggers = self.get_triggers("time/time_trigger_words.yaml")
        date_triggers = self.get_triggers("date/date_trigger_words.yaml")
        separator_fst = self.common.optional_delete_space_fst + self.common.optional_delete_article_fst + \
            pynutil.delete(time_triggers | date_triggers).closure() + self.common.optional_delete_space_fst
        separator_fst = separator_fst.optimize()
        return separator_fst

    def get_pure_datetime(self):
        separator_fst = self.get_separators()
        functions_with_params = {
            "date_fst": [DateFst("date", self.dictionaries_path, None).get_pure_date],
            "time_fst": [TimeFst("time", self.dictionaries_path, None).get_pure_time]
        }
        models = utils.parallelize_functions(functions_with_params)
        date_fst = models["date_fst"] @ self.common.remove_sharp
        time_fst = models["time_fst"] @ self.common.remove_sharp
        date_fst = pynutil.insert(utils.make_json_key(JsonKeys.date)) + date_fst + pynutil.insert(",")
        time_fst = pynutil.insert(utils.make_json_key(JsonKeys.time)) + time_fst + pynutil.insert(",")

        functions_with_params = {
            "optimized_date_time": [self.get_optimized_date_time, date_fst, separator_fst, time_fst],
            "optimized_time_date": [self.get_optimized_time_date, time_fst, separator_fst, date_fst],
            "optimized_date": [self.get_optimized_date, date_fst],
            "optimized_time": [self.get_optimized_time, time_fst]
        }
        models = utils.parallelize_functions(functions_with_params)
        result_fst = pynini.union(*models.values())
        result_fst = pynutil.insert("{") + result_fst + pynutil.insert("}")
        return result_fst

    def create_model(self):
        datetime_fst = self.get_pure_datetime()
        datetime_fst = pynini.closure(pynutil.add_weight(self.common.alphabet, -1)).optimize() @ datetime_fst
        datetime_fst = self.common.replace_with_sharp + datetime_fst + self.common.replace_with_sharp
        datetime_fst = self.common.delete_extra_space @ self.common.doubled_form_fst @ datetime_fst
        self.model = datetime_fst.optimize()
