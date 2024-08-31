#!/usr/bin/env python
# encoding: utf-8
from itertools import starmap
from cat_to_wiki import load_sheets
from pandas import DataFrame, read_excel
import json

# make validation json


def excel_to_dict_array(path, mapper=lambda pk, record: dict(record.to_dict(), id=pk), sheetname=0):
    """
    Читает таблицу xlsx и возращает в виде списка диктов
    :param str path: Путь к файлу xlsx
    :param function mapper: Функция, преобразующая полные дикты представляющие строки в нужный формат
                            На вход принимает ключ (значение первой колонки) и pandas.core.series.Series
    :param num|str sheetname: Номер или имя листа в файле
    :return list[dict]:
    """
    sheets = load_sheets(path)
    if isinstance(sheetname, basestring):
        dataframe = sheets[sheetname]
    else:
        dataframe = sheets.values()[0]

    return list(starmap(mapper, dataframe.iterrows()))
    #with open(path) as inp:
    #    dframe = read_excel(inp, sheetname=sheetname)
    #    return dframe


#inp_path = 'tmp/validation.xlsx'
inp_path = '/Users/yoschi/Voice/tasks/DIALOG-950/validation_filtered.xlsx'
def mapper(pk, record):
    return {
        "context_0": record.context_0,
        "context_1": record.context_1,
        "context_2": record.context_2,
        "reply": record.reply,
        "key": pk,
        "true": record.reference_result,
    }


out_path = 'tmp/context_validation.json'
print json.dump(excel_to_dict_array(inp_path, mapper), open(out_path, 'w'))
