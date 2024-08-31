#!/usr/bin/env python
# encoding: utf-8
#!/usr/bin/env python
# encoding: utf-8
from random import shuffle

from utils.nirvana.op_caller import call_as_operation

from utils.toloka.split import split_goldenset, split_tasks

from intent_label.src.sync_prj import IntentProjectRequester
from intent_label.src.run_pool import IntentPoolRequester


# Запускает синк настроек Разметки интентов и создаёт пул


def setup_logging():
    import logging
    root_logger = logging.getLogger('')
    root_logger.setLevel(logging.INFO)

    # Выключаем надоедливые "InsecurePlatformWarning"
    import requests.packages.urllib3
    requests.packages.urllib3.disable_warnings()


def create_pool(prj_key, records):
    prj_req = IntentProjectRequester(prj_key)
    prj_req.check_layout_skill_ids()

    pool_req = IntentPoolRequester(prj_key)
    gs_records = pool_req.list_honeypots()  # Здесь ещё и валидируются хинты и ханипоты

    prj_req.sync_skill_pools()
    prj_req.sync_prj_props()

    pool_req.create_pool()
    pool_id = pool_req.pool_id

    honeypots, validation_pool, validation_map = split_goldenset(gs_records, None, None, pool_id)
    tasks, additions = split_tasks(records, prj_req.prj_conf.get_visible_fields(), pool_id, group_matching=True)
    tasks.extend(validation_pool)
    shuffle(tasks)
    return {
        'tasks': tasks,
        'additions': additions,
        'honeypots': honeypots,
        'validation': validation_map,
        'env': {"environment": pool_req.get_env_name()},  # Для перезаписи опции в хитман-кубиках
    }


if __name__ == '__main__':
    setup_logging()
    call_as_operation(create_pool, {
        'records': {'parser': 'json'},
    })
