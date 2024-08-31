# -*-coding: utf8 -*-
#!/usr/bin/env python

from os.path import basename as path_basename
from utils.nirvana.op_caller import call_as_operation


def get_reqid(filename):
    return path_basename(filename).strip('.png')


def get_reqid_suffix(reqid):
    return '-'.join(reqid.split('-')[-2:])


def make_reqid_to_screenshot_dict(mds_screenshots):
    return dict((get_reqid(item["initialFileName"]), item["downloadUrl"]) for item in mds_screenshots)


def make_reqid_to_hashsum_dict(hashsum):
    return dict((get_reqid(name), hashsum) for name, hashsum in hashsum.items())


def make_layouts(first_system, second_system, honeypots, first_hashsum, second_hashsum, hp_hashsum):
    layouts, screens = [], []

    for req_id, screenshot in first_system.items():
        if req_id not in second_system:
            continue
        if first_hashsum and second_hashsum:
            if first_hashsum[req_id] == second_hashsum[req_id]:
                continue
            if hp_hashsum:
                if first_hashsum[req_id] == hp_hashsum[req_id] or second_hashsum[req_id] == hp_hashsum[req_id]:
                    continue

        layout = {}
        layout["screens"] = [
            {"origUrl": screenshot},
            {"origUrl": second_system[req_id]}
        ]
        if honeypots:
            if req_id not in honeypots:
                continue
            layout["honeypots"] = [
                {"origUrl": honeypots[req_id]}
            ]

        layouts.append(layout)
        screens.append({"name": "screen_" + get_reqid_suffix(req_id)})

    return screens, layouts


def main(first_system_mds_screenshots, second_system_mds_screenshots, hp_mds_screenshots=None,
         good_tasks=5, bad_tasks=1, approve_mode="no-approve", abc_id=23708, overlap=11,
         title="Alice experiment", description="SBS на скриншотах Алисы", question=None,
         first_sys_name="baseline", second_sys_name="new_system",
         first_hashsum=None, second_hashsum=None, hp_hashsum=None):
    """
    Код для генерации sbs в формате макетного эксперимента для сравнения двух систем.
    Подробнее здесь: https://wiki.yandex-team.ru/sbs/api/#sozdatnovujuzajavkumaketnyjjjeksperiment
    Пример запуска: https://nirvana.yandex-team.ru/flow/cade6ec2-dce0-4376-9612-6ef9df91e134/763136c4-7ef0-48e0-869a-83ddd2cb1f20/graph
    (увеличивайте ttl у последнего кубика)
    :param first_system_mds_screenshots: скриншоты с mds, обязательно с https
    :param second_system_mds_screenshots: скриншоты с mds, обязательно с https
    :param hp_system_mds_screenshots: скриншоты-ханипоты с mds, обязательно с https
    :param title: название эксперимента (внутреннее)
    :param description: описание эксперимента (внутреннее)
    :param question: вопрос, который задаётся толокерам (внешнее)
    :param first_sys_name: название первой системы (внутреннее)
    :param second_sys_name: название второй системы (внутреннее)
    :param first_hashum: hashsum кубика render div cards для первой системы (для фильтрации одинаковых изображений)
    :param second_hashsum: hashsum кубика render div cards для второй системы (для фильтрации одинаковых изображений)
    :param hp_hashsum: hashsum кубика render div cards для ханипотов (для фильтрации одинаковых изображений)
    :param good_tasks: кол-во обычных пар в тасксьюте
    :param bad_tasks: кол-во пар-ханипотов в тасксьюте
    :param approve_mode: нужен ли апрув аналитика для запуска
    :return: json in sbs maket format
    """

    first_system = make_reqid_to_screenshot_dict(first_system_mds_screenshots)
    second_system = make_reqid_to_screenshot_dict(second_system_mds_screenshots)
    honeypots = None

    if first_hashsum and second_hashsum:
        first_hashsum = make_reqid_to_hashsum_dict(first_hashsum)
        second_hashsum = make_reqid_to_hashsum_dict(second_hashsum)
    if hp_mds_screenshots:
        honeypots = make_reqid_to_screenshot_dict(hp_mds_screenshots)
        if hp_hashsum:
            hp_hashsum = make_reqid_to_hashsum_dict(hp_hashsum)

    screens, layouts = make_layouts(first_system, second_system, honeypots, first_hashsum, second_hashsum, hp_hashsum)

    res_data = {
        "type": "layout",
        "approveMode": approve_mode,
        "experiment": {
            "title": title,
            "description": description,
            "abcService": abc_id,
            "poolTitle": "touch_360",
            "goodTasks": good_tasks,
            "badTasks": bad_tasks,
            "overlap": {
                "mode": "edit",
                "value": overlap
            },
            "layouts": {
                "systems": [
                    {"name": first_sys_name},
                    {"name": second_sys_name}
                ],
                "screens": screens,
                "layouts": layouts
            },
        }
    }
    if question:
        res_data["experiment"]["question"] = question

    return res_data


if __name__ == '__main__':
    call_as_operation(
        main,
        input_spec={
            'first_system_mds_screenshots': {'link_name': 'first_mds', 'required': True, 'parser': 'json'},
            'second_system_mds_screenshots': {'link_name': 'second_mds', 'required': True, 'parser': 'json'},
            'hp_mds_screenshots': {'link_name': 'hp_mds', 'required': False, 'parser': 'json'},
            'first_hashsum': {'link_name': 'first_hashsum', 'required': False, 'parser': 'json'},
            'second_hashsum': {'link_name': 'second_hashsum', 'required': False, 'parser': 'json'},
            'hp_hashsum': {'link_name': 'hp_hashsum', 'required': False, 'parser': 'json'}
        }
    )
