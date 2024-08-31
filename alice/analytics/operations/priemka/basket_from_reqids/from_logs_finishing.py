# -*-coding: utf8 -*-
import time
from os import path
from qb2.api.v1 import typing as qt, filters as qf, extractors as qe
from nile.api.v1 import filters as nf, extractors as ne, aggregators as na, Record, with_hints

from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps
import utils.yt.basket_common as common

import nirvana.job_context as nv
import json


def main(data, output_table=None, pool=None, tmp_path=None, memory_limit=1024,
         store_checkpoints=False, mr_account=None):

    with open(data, 'r') as f:
        input_table = json.load(f)
    input_table = input_table['table']

    random_part = common.random_part(16)
    owner = nv.context().get_meta().get_owner()

    if not output_table:
        if mr_account:
            output_table = "//home/{}/{}/nirvana/{}".format(mr_account, owner, random_part)
        else:
            output_table = "//tmp/robot-voice-qa/new_scenarios_basket_" + random_part

    templates = {"job_root": "//tmp/robot-voice-qa",
                 'checkpoints_root': path.dirname(output_table)
                 }
    if tmp_path is not None:
        templates['tmp_files'] = tmp_path


    """ инициализация доступа к кластеру"""
    cluster = hahn_with_deps(templates=templates, include_utils=True, pool=pool, use_yql=True)
    job = cluster.job().env(default_memory_limit=memory_limit,)

    """ opening input stream """
    all_requests_with_audio_and_device_states = job.table(input_table)

    """ генерируем фейковые req_ids и session_ids """
    interim_basket_with_fake_ids = all_requests_with_audio_and_device_states \
        .project(ne.all(),
                 new_req_id=ne.custom(common.generate_id, 'req_id').with_type(qt.Optional[qt.String])
                 )
    session_ids = interim_basket_with_fake_ids \
        .groupby('main_req_id') \
        .reduce(
            with_hints(output_schema=dict(main_req_id=qt.Optional[qt.String],
                                      new_session_id=qt.Optional[qt.String], ))
            (common.get_session_ids)
        ) \
        .filter(nf.custom(lambda session_id: len(session_id.split('__')) <= 2, 'new_session_id'))

    basket_with_fake_ids = interim_basket_with_fake_ids \
        .join(session_ids, by='main_req_id')

    """записываем таблицу в расширенном сыром формате"""
    raw_table = output_table + '_raw'
    basket_with_fake_ids.put(raw_table)

    """ обрабатываем выходные поля """
    basket = basket_with_fake_ids \
        .project(
            ne.all(exclude=('request_additional_options')),
            additional_options=ne.custom(common.clean_additional_options,
                                     'request_additional_options').with_type(qt.Optional[qt.Json]),
            toloka_intent=ne.const('other')
        ) \
        .checkpoint(random_part + '_basket_before_qb2') \
        .qb2(
            log='generic-yson-log',
            fields=common.get_basket_fields(do_not_change_ss=True)
        )

    basket.put(output_table)

    with job.driver.transaction():
        job.run(store_checkpoints=store_checkpoints)

    return [{"cluster": "hahn", "table": output_table}, {"cluster": "hahn", "table": raw_table}]

if __name__ == '__main__':
    st = time.time()
    print 'start at', time.ctime(st)
    call_as_operation(main, input_spec={"data": {}})
    print 'total elapsed {:2f} min'.format((time.time() - st) / 60)
