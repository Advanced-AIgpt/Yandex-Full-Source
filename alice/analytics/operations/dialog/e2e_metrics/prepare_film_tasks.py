#!/usr/bin/env python
# encoding: utf-8

import sys
import argparse
import random

from nile.api.v1 import (
    clusters,
    Record
)

from utils.nirvana.op_caller import call_as_operation


def get_url(record):
    if record['video_gallery.provider_name'] == 'amediateka':
        if record['video_gallery.type'] == 'tv_show':
            if record.get('video_gallery.human_readable_id', "") != "":
                return "https://www.amediateka.ru/serial/" + record["video_gallery.human_readable_id"]
            else:
                return "https://www.amediateka.ru/serial/" + record["video_gallery.provider_item_id"]
        else:
            if record.get('video_gallery.human_readable_id', "") != "":
                return "https://www.amediateka.ru/film/" + record["video_gallery.human_readable_id"]
            else:
                return "https://www.amediateka.ru/film/" + record["video_gallery.provider_item_id"]
    elif record['video_gallery.provider_name'] == 'ivi':
        if record.get('video_gallery.human_readable_id', "") != "":
            return "https://www.ivi.ru/watch/" + record['video_gallery.human_readable_id']
        else:
            return "https://www.ivi.ru/watch/" + record["video_gallery.provider_item_id"] + '/description'
    elif record['video_gallery.provider_name'] == 'youtube':
        return "https://www.youtube.com/watch?v=" + record["video_gallery.provider_item_id"]
    else:
        return ""


def get_film_tasks(records):
    for rec in records:
        if rec['video_gallery.index'] > 4:
            continue
        obj = {'hyp': rec['utterance'],
               'reply': rec['video_gallery.name'],
               'position': rec['video_gallery.index']
              }
        if rec['intent_name'] != 'personal_assistant.scenarios.video_play':
            obj['result'] = 'not_video'
            yield Record(**obj)
            continue
        else:
            obj['video'] = get_url(rec)
            if obj['video'] == '':
                obj['result'] = 'url_error'
            yield Record(**obj)


def process_joined_urls(records):
    for rec in records:
        if ('result' in rec) or ('reply' in rec):
            yield rec
        else:
            yield Record(rec, result='not_video')


def main(input_table, data_table, output_table):
    #output_table = '//tmp/station_film_e2e_' + str(random.randint(0, 1000000))
    cluster = clusters.YT(proxy="hahn.yt.yandex.net") \
        .env(templates=dict(
            job_root=('tmp')))
    job = cluster.job()
    urls = job.table(input_table) \
              .map(get_film_tasks)
    if data_table:
        data = job.table(data_table) \
                  .join(urls, type='left', by='hyp') \
                  .map(process_joined_urls) \
                  .put(output_table)
    else:
        urls.put(output_table)
    job.run()

    return {"cluster": "hahn", "table": output_table}


if __name__ == '__main__':
    call_as_operation(main)
