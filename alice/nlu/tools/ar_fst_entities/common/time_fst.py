from alice.nlu.tools.ar_fst_entities.common import utils
from alice.nlu.tools.ar_fst_entities.common.base_fst import BaseFst
from alice.nlu.tools.ar_fst_entities.common.json_keys import JsonKeys
from alice.nlu.tools.ar_fst_entities.common.number_fst import NumberFst
import pynini
from pynini.lib import pynutil


class TimeFst(BaseFst):
    def __init__(self, name, dictionaries_path, path):
        super(TimeFst, self).__init__(name, dictionaries_path, path)
        self.hours_to_fst = utils.fst_from_tsv_file(self.dictionaries_path, "time/hour_to.tsv").optimize()
        self.minutes_to_fst = utils.fst_from_tsv_file(self.dictionaries_path, "time/minute_to.tsv").optimize()
        hour_word = utils.load_dict(self.dictionaries_path, "time/hour_word.yaml")
        self.hour_word_fst = pynini.string_map(hour_word["singular"] + hour_word["plural"]).optimize()
        self.delete_hour_word_fst = pynutil.delete(self.hour_word_fst)
        self.optional_delete_hour_word_fst = self.delete_hour_word_fst.ques
        minute_word = utils.load_dict(self.dictionaries_path, "time/minute_word.yaml")
        self.minute_word_fst = pynini.string_map(minute_word["singular"] + minute_word["plural"]).optimize()
        self.delete_minute_word_fst = pynutil.delete(self.minute_word_fst)
        self.optional_delete_minute_word_fst = self.delete_minute_word_fst.ques
        second_word = utils.load_dict(self.dictionaries_path, "time/second_word.yaml")
        self.second_word_fst = pynini.string_map(second_word["singular"] + second_word["plural"]).optimize()
        self.delete_second_word_fst = pynutil.delete(self.second_word_fst)
        self.delete_time_separators = pynutil.delete(
            utils.fst_from_tsv_file(self.dictionaries_path, "time/time_separators.tsv")) | self.common.delete_waw_fst
        self.delete_time_separators = self.delete_time_separators.optimize()
        self.optional_day_part_fst = self.common.day_part_fst.ques
        self.hour_part_fst = utils.fst_from_dict_file(self.dictionaries_path, "time/hour_part.yaml")
        self.hour_part_fst = self.hour_part_fst.optimize()
        time_triggers = utils.load_dict(self.dictionaries_path, "time/time_trigger_words.yaml")
        self.pre_relative_fst =(
            pynini.string_map(time_triggers["pre_relative"]).closure(1) @ self.common.replace_with_sharp).optimize()
        word_every_fst = utils.fst_from_tsv_file(self.dictionaries_path, "common/word_every.tsv")
        self.time_repeat_fst = pynutil.delete(word_every_fst)
        self.time_repeat_fst @= self.common.replace_with_sharp
        self.time_repeat_fst = self.time_repeat_fst.optimize()
        self.illa_fst = utils.fst_from_tsv_file(self.dictionaries_path, "time/illa_word.tsv")
        self.delete_illa_fst = pynutil.delete(self.illa_fst).optimize()

    def get_absolute_time_fst(self, number_fst):
        hours_fst = pynutil.insert(utils.make_json_key(JsonKeys.hours), weight=-2) + \
            number_fst + self.common.optional_delete_space_fst + self.optional_delete_hour_word_fst
        hours_fst += pynutil.insert(",")
        minutes_fst = pynutil.insert(utils.make_json_key(JsonKeys.minutes), weight=-2)
        minutes_fst += number_fst + self.common.optional_delete_space_fst + self.optional_delete_minute_word_fst
        minutes_fst += pynutil.insert(",")
        temp_fst = pynutil.insert(utils.make_json_key(JsonKeys.minutes), weight=-2)
        temp_fst += self.common.optional_delete_space_fst + self.common.optional_delete_article_fst + self.hour_part_fst
        temp_fst += pynutil.insert(",")
        minutes_fst |= temp_fst
        seconds_fst = pynutil.insert(utils.make_json_key(JsonKeys.seconds), weight=-2)
        seconds_fst += number_fst + self.common.optional_delete_space_fst + self.delete_second_word_fst
        seconds_fst += pynutil.insert(",")
        time_main_part_fst = hours_fst + self.common.optional_delete_space_fst + self.delete_time_separators + \
            self.common.optional_delete_space_fst + minutes_fst + self.common.optional_delete_space_fst + \
            self.delete_time_separators + self.common.optional_delete_space_fst + seconds_fst
        time_main_part_fst |= hours_fst + self.common.optional_delete_space_fst + self.delete_time_separators + \
            self.common.optional_delete_space_fst + minutes_fst
        time_main_part_fst |= minutes_fst + self.common.optional_delete_space_fst + self.delete_time_separators + \
            self.common.optional_delete_space_fst + seconds_fst
        time_main_part_fst |= hours_fst | minutes_fst | seconds_fst
        result_fst = pynutil.insert("{")
        result_fst += pynutil.insert(utils.make_json_key_value(JsonKeys.is_relative, "false")) + \
            self.common.optional_delete_space_fst + time_main_part_fst + self.common.optional_delete_space_fst + \
            self.optional_day_part_fst + pynutil.insert("}")
        return result_fst.optimize()

    def get_absolute_time_to_fst(self, number_fst):
        hours_to_final_fst = pynutil.insert(utils.make_json_key(JsonKeys.hours), weight=-2) + (
            number_fst @ self.hours_to_fst) + pynutil.insert(",")
        minutes_to_final_fst = pynutil.insert(utils.make_json_key(JsonKeys.minutes), weight=-2)
        minutes_to_final_fst += (
            number_fst @ self.minutes_to_fst) + self.common.optional_delete_space_fst + \
            self.optional_delete_minute_word_fst
        minutes_to_final_fst += pynutil.insert(",")
        temp_fst = pynutil.insert(utils.make_json_key(JsonKeys.minutes), weight=-2)
        temp_fst += ((self.common.optional_delete_article_fst + self.hour_part_fst) @ self.minutes_to_fst) + \
            pynutil.insert(",")
        minutes_to_final_fst |= temp_fst
        time_to_main_part_fst = hours_to_final_fst + self.common.optional_delete_space_fst + \
            self.delete_illa_fst + self.common.optional_delete_space_fst + minutes_to_final_fst
        result_fst = pynutil.insert("{")
        result_fst += pynutil.insert(utils.make_json_key_value(JsonKeys.is_relative, "false")) + \
            self.common.optional_delete_space_fst + time_to_main_part_fst + self.common.optional_delete_space_fst + \
            self.optional_day_part_fst + pynutil.insert("}")
        return result_fst.optimize()

    def get_relative_time_fst(self, number_fst):
        relative_hours_fst = pynutil.insert(utils.make_json_key(JsonKeys.hours), weight=-2)
        relative_hours_fst += self.common.optional_delete_waw_fst + self.common.optional_delete_space_fst + \
            number_fst + self.common.optional_delete_space_fst + self.delete_hour_word_fst
        relative_hours_fst += pynutil.insert(",")
        temp_fst = pynutil.insert(utils.make_json_key(JsonKeys.hours), weight=-2)
        temp_fst += self.common.optional_delete_waw_fst + self.common.optional_delete_space_fst + \
            pynini.cross(self.hour_word_fst, "1") + self.common.optional_delete_space_fst
        temp_fst += pynutil.insert(",")
        relative_hours_fst |= temp_fst
        relative_minutes_fst = pynutil.insert(utils.make_json_key(JsonKeys.minutes), weight=-2)
        relative_minutes_fst += self.common.optional_delete_space_fst + number_fst + \
            self.common.optional_delete_space_fst + self.delete_minute_word_fst
        relative_minutes_fst += pynutil.insert(",")
        temp_fst = pynutil.insert(utils.make_json_key(JsonKeys.minutes), weight=-2)
        temp_fst += self.common.optional_delete_space_fst + pynini.cross(self.minute_word_fst, "1") + \
            self.common.optional_delete_space_fst
        temp_fst += pynutil.insert(",")
        relative_minutes_fst |= temp_fst
        temp_fst = pynutil.insert(utils.make_json_key(JsonKeys.minutes), weight=-2)
        temp_fst += self.common.optional_delete_space_fst + self.common.optional_delete_article_fst + self.hour_part_fst
        temp_fst += pynutil.insert(",")
        relative_minutes_fst |= temp_fst
        relative_seconds_fst = pynutil.insert(utils.make_json_key(JsonKeys.seconds), weight=-2)
        relative_seconds_fst += self.common.optional_delete_space_fst + number_fst + \
            self.common.optional_delete_space_fst + self.delete_second_word_fst
        relative_seconds_fst += pynutil.insert(",")
        temp_fst = pynutil.insert(utils.make_json_key(JsonKeys.seconds), weight=-2)
        temp_fst += self.common.optional_delete_space_fst + pynini.cross(self.second_word_fst, "1") + \
            self.common.optional_delete_space_fst
        temp_fst += pynutil.insert(",")
        relative_seconds_fst |= temp_fst
        relative_time_main_part_fst = relative_hours_fst + self.common.optional_delete_space_fst + \
            self.delete_time_separators + self.common.optional_delete_space_fst + \
            relative_minutes_fst + self.common.optional_delete_space_fst + \
            self.delete_time_separators + self.common.optional_delete_space_fst + \
            relative_seconds_fst
        relative_time_main_part_fst |= relative_hours_fst + self.common.optional_delete_space_fst + \
            self.delete_time_separators + self.common.optional_delete_space_fst + \
            relative_minutes_fst
        relative_time_main_part_fst |= relative_minutes_fst + self.common.optional_delete_space_fst + \
            self.delete_time_separators + self.common.optional_delete_space_fst + \
            relative_seconds_fst
        relative_time_main_part_fst |= relative_hours_fst | relative_minutes_fst | relative_seconds_fst

        temp_fst = pynutil.insert(utils.make_json_key(JsonKeys.hours), weight=-2)
        temp_fst += number_fst + pynutil.insert(",") + self.delete_time_separators
        temp_fst += pynutil.insert(utils.make_json_key(JsonKeys.minutes), weight=-2)
        temp_fst += number_fst + pynutil.insert(",")
        temp_fst += (pynutil.insert(utils.make_json_key(JsonKeys.seconds),
                                    weight=-2) + self.delete_time_separators + number_fst + pynutil.insert(",")).ques
        relative_time_main_part_fst |= temp_fst
        result_fst = ((self.pre_relative_fst + pynutil.insert("{")) | (
            self.time_repeat_fst + pynutil.insert("{") + pynutil.insert(
                utils.make_json_key_value(JsonKeys.repeat, "true"))))
        result_fst += pynutil.insert(utils.make_json_key_value(JsonKeys.is_relative, "true")) + \
            self.common.optional_delete_space_fst + relative_time_main_part_fst + self.common.optional_delete_space_fst + \
            pynutil.insert("}")
        return result_fst.optimize()

    def get_relative_time_to_fst(self, number_fst):
        relative_hours_to_final_fst = pynutil.insert(utils.make_json_key(JsonKeys.hours), weight=-2)
        relative_hours_to_final_fst += (
            number_fst @ self.hours_to_fst) + self.common.optional_delete_space_fst + \
            self.delete_hour_word_fst
        relative_hours_to_final_fst += pynutil.insert(",")
        relative_minutes_to_final_fst = pynutil.insert(utils.make_json_key(JsonKeys.minutes), weight=-2)
        relative_minutes_to_final_fst += (
            number_fst @ self.minutes_to_fst) + self.common.optional_delete_space_fst + \
            self.delete_minute_word_fst
        relative_minutes_to_final_fst += pynutil.insert(",")
        temp_fst = pynutil.insert(utils.make_json_key(JsonKeys.minutes), weight=-2)
        temp_fst += ((self.common.optional_delete_article_fst + self.hour_part_fst) @ self.minutes_to_fst) + \
            pynutil.insert(",")
        relative_minutes_to_final_fst |= temp_fst
        relative_time_to_main_part_fst = relative_hours_to_final_fst + self.common.optional_delete_space_fst + \
            self.delete_illa_fst + self.common.optional_delete_space_fst + \
            relative_minutes_to_final_fst
        result_fst = ((self.pre_relative_fst + pynutil.insert("{")) | (
            self.time_repeat_fst + pynutil.insert("{") + pynutil.insert(
                utils.make_json_key_value(JsonKeys.repeat, "true"))))
        result_fst += pynutil.insert(utils.make_json_key_value(JsonKeys.is_relative, "true")) + \
            self.common.optional_delete_space_fst + relative_time_to_main_part_fst + self.common.optional_delete_space_fst + \
            pynutil.insert("}")
        return result_fst.optimize()

    def get_pure_time(self):
        number_fst = NumberFst.get_static_pure_number(self.dictionaries_path, 6)
        functions_with_params = {
            "absolute_time_fst": [self.get_absolute_time_fst, number_fst],
            "absolute_time_to_fst": [self.get_absolute_time_to_fst, number_fst],
            "relative_time_fst": [self.get_relative_time_fst, number_fst],
            "relative_time_to_fst": [self.get_relative_time_to_fst, number_fst]
        }
        models = utils.parallelize_functions(functions_with_params)
        result_fst = pynini.union(*models.values())
        return result_fst.optimize()

    def create_model(self):
        time_final_fst = self.get_pure_time()
        time_final_fst = pynini.closure(pynutil.add_weight(
            self.common.alphabet, -1)).optimize() @ time_final_fst
        time_final_fst = self.common.replace_with_sharp + time_final_fst + self.common.replace_with_sharp
        time_final_fst = self.common.delete_extra_space @ self.common.doubled_form_fst @ time_final_fst
        self.model = time_final_fst.optimize()
