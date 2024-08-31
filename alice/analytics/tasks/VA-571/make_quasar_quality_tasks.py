# -*-coding: utf8 -*-
from nile.api.v1 import Record, aggregators as na, extractors as ne
from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps

from visualize_quasar_sessions import get_json_tasks, random_hash

from math import sqrt
from random import random
from re import findall
from functools import partial


def get_list_records(records):  # some rows in one row to next calculating
    intents, counts = [], []
    for rec in records:
        intents.append(rec['intent'])
        counts.append(rec['count'])
    yield Record(**{"intents": intents, "counts": counts})


def sampler(records):  # probability sampling with P = (sampling_size / all_count) for each intent
    for record in records:
        random_value = random()
        if record.get('sample_rate') and random_value < record['sample_rate']:
            yield record


def modify_intent(intent):  # merge many handcrafted scenarios in one handcrafted except cancel
    if intent.startswith("personal_assistant\thandcrafted") and intent != "personal_assistant\thandcrafted\tcancel":
        return "personal_assistant\thandcrafted\thandcrafted"
    return intent


def get_sample_sizes(records, min_size, size):
    for record in records:
        small_count, handcrafted_count = 0, 0
        large = []
        for intent, count in zip(record['intents'], record['counts']):
            if count < min_size or findall(r'external_skill|market|call|internal|image_what_is_this|taxi', intent):
                continue
            if intent.startswith("personal_assistant\thandcrafted") and not intent.endswith("handcrafted\tcancel"):
                handcrafted_count += count
            else:
                if count <= min_size ** 2:  # sample_size = min_size if count <= min_size**2
                    small_count += min_size
                    yield Record(**{"modified_intent": intent, "sample_rate": (min_size + 0.) / count})
                else:
                    large.append({"intent": intent, "sample_size": sqrt(count), "count": count})
        large.append(  # for each intent sample_size = sqrt(count) if count > min_size**2
            {"intent": "personal_assistant\thandcrafted\thandcrafted", "sample_size": sqrt(handcrafted_count),
             "count": handcrafted_count})

        multiplier = (size - small_count) / sum(map(lambda x: x['sample_size'], large))
        for rec in large:
            new_sample_size = int(round(rec['sample_size'] * multiplier))
            yield Record(**{"modified_intent": rec['intent'], "sample_rate": (new_sample_size + 0.) / rec["count"]})


def main(start_date, end_date, size=7000, min_size=25, input_table=None, pool=None, tmp_path=None,
         sessions_root='//tmp/robot-voice-qa/quasar_sessions', output_table=None, long_sessions=False,
         memory_limit=16384, do_sampling=True):

    if input_table:
        date_str = input_table.split('/')[-1]
    else:
        date_str = '{' + start_date + '..' + end_date + '}'

    templates = dict(job_root='//tmp/robot-voice-qa', date=date_str)

    if tmp_path is not None:
        templates['tmp_files'] = tmp_path

    cluster = hahn_with_deps(pool=pool,
                             templates=templates,
                             neighbours_for=__file__,
                             neighbour_names=['visualize_quasar_sessions.py', 'generic_scenario_to_human_readable.py',
                                              'intents_to_human_readable.py', 'standardize_answer.py'])

    io_option = {"table_writer": {"max_row_weight": 134217728}}
    job = cluster.job().env(yt_spec_defaults={"job_io": io_option, "partition_job_io": io_option,
                                              "merge_job_io": io_option, "map_job_io": io_option,
                                              "reduce_job_io": io_option, "sort_job_io": io_option},
                            default_memory_limit=memory_limit)

    output_table = output_table or "//tmp/robot-voice-qa/quasar_quality_task_" + random_hash()
    input_table_path = input_table or sessions_root+'/@date'

    tasks_table, long_sessions_table = job.table(input_table_path) \
        .map(get_json_tasks)  # here get json tasks

    if long_sessions:
        tasks_table = long_sessions_table

    if do_sampling:
        samples_sizes = tasks_table \
            .project('intent') \
            .groupby('intent') \
            .aggregate(count=na.count()) \
            .map(get_list_records, intensity='large_data') \
            .map(partial(get_sample_sizes, min_size=min_size, size=size))  # here get table with mapping modified_intent -> sampling_rate

        result_table = tasks_table.project(ne.all(), modified_intent=ne.custom(modify_intent, 'intent')) \
            .join(samples_sizes, by='modified_intent', type='left', assume_unique_right=True) \
            .map(sampler) \
            .sort('sorting_hash') \
            .project(ne.all(['sorting_hash', 'sample_rate', 'modified_intent']))
    else:
        result_table = tasks_table

    result_table.put(output_table)

    job.run()
    return {"cluster": "hahn", "table": output_table}


if __name__ == '__main__':
    call_as_operation(main)
