from alice.nlu.tools.ar_fst_entities.common.date_fst import DateFst
from alice.nlu.tools.ar_fst_entities.common.datetime_fst import DatetimeFst
from alice.nlu.tools.ar_fst_entities.common.datetime_range_fst import DatetimeRangeFst
from alice.nlu.tools.ar_fst_entities.common.float_fst import FloatFst
from alice.nlu.tools.ar_fst_entities.common.number_fst import NumberFst
from alice.nlu.tools.ar_fst_entities.common.time_fst import TimeFst
from alice.nlu.tools.ar_fst_entities.common.selection_fst import SelectionFst
from alice.nlu.tools.ar_fst_entities.common.weekdays_fst import WeekdaysFst


def create_fst_object(name, dictionaries_path, path):
    fst_factory = {
        "number": NumberFst,
        "time": TimeFst,
        "date": DateFst,
        "weekdays": WeekdaysFst,
        "selection": SelectionFst,
        "float": FloatFst,
        "datetime": DatetimeFst,
        "datetime_range": DatetimeRangeFst}
    if name not in fst_factory:
        raise ValueError("Unknown fst name: {}".format(name))
    return fst_factory[name](name, dictionaries_path, path)
