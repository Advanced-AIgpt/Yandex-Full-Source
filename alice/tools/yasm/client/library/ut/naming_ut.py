# -*- coding: utf-8 -*-
"""
Тестирование функций именования
"""
from typing import Dict
from library.python import resource
from json import loads as j_loads
from alice.tools.yasm.client.library.common import gen_full_alert_name


# -------------------------------------------------------------------------------------------------
def test_get_full_alert_name():
    objects: Dict = j_loads(resource.find("/common/objects.json"))
    results: Dict = j_loads(resource.find("/naming/results.json"))
    for data in zip(objects, results["full_alert_names"]):
        assert gen_full_alert_name(objects[data[0]], "test_alert") == data[1]
