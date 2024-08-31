#!/usr/bin/env python
# encoding: utf-8
"""
Настройки доступов к пулам
"""


def skill_filter(skill_id, options):
    """
    Генерит фильтр по скиллу для api
    :param str skill_id:
    :param dict[str, int|None] options: {"OPERATOR": value}
    :return: {"or" [набор фильтров для скила]}
    """
    filters = []
    for operator, value in options.iteritems():
        filters.append({"operator": operator,
                        "category": "skill",
                        "value": value,  # FIXME: to str?
                        "key": skill_id})

    return {"or": filters}


RU_LANG_FILTER = {"or": [{
    "operator": "IN",
    "category": "profile",
    "key": "languages",
    "value": "RU",
}]}


def rkub_filter():
    # Принадлежность пользователя к ближайшим русскоговорящим странам (РКУБ) + Молдова/Латвия/Литва
    regions = []
    for country_id in [225, 187, 149, 159, 208, 117, 206]:
        regions.append({"operator": "IN",
                        "category": "computed",
                        "key": "region_by_phone",
                        "value": country_id})

    return {"or": regions}
