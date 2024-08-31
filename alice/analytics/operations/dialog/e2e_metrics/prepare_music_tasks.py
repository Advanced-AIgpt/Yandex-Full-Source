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


def get_url(answer):
    if answer.get('uri', None) is None:
        if 'first_track' in answer:
            uri = answer['first_track']['uri'].replace('yandexmusic://', '') \
                                            .replace('https://music.yandex.ru/', '') \
                                            .replace('?from=alice&mob=0', '')
        else:
            return 'not_music'
    elif not (answer['uri'].startswith('yandexmusic') or 
                answer['uri'].startswith('https://music.yandex.ru/')):
        return 'not_music_'+answer['uri']
    else:
        uri = answer['uri'].replace('yandexmusic://', '') \
                            .replace('https://music.yandex.ru/', '') \
                            .replace('?from=alice&mob=0', '')
    parts = uri.split('/')
    if 'album' in uri:
        if 'track' in uri:
            return '/'.join([parts[2], parts[3], parts[1]]) + '/'
        else:
            return uri
    if 'artist' in uri:
        return uri
    if 'playlists' in uri:
        return 'playlist/' + '/'.join([parts[1],parts[3]]) + '/show/cover/description/'
    return ''


def get_music_tasks(records):
    for rec in records:
        obj = {'hyp': rec['utterance'],
               'reply': rec['text_answer']
              }
        if rec['intent_name'] != 'personal_assistant.scenarios.music_play':
            obj['result'] = 'not_music'
            yield Record(**obj)
            continue
        elif rec.get('form.answer', None) is None:
            obj['result'] = 'not_found'
            yield Record(**obj)
            continue
        else:
            url = get_url(rec['form.answer'])
            if url == '' or url.startswith('not_music'):
                obj['url'] = ''
                obj['result'] = 'not_found'
                yield Record(**obj)
                continue

            obj['url'] = url
            obj['alice_reply'] = rec['text_answer']
            if 'artist' in url:
                obj['music_ref'] = 'https://music.yandex.ru/' + url
            yield Record(**obj)


def main(input_table, data_table, output_table):
    #output_table = '//tmp/station_music_e2e_' + str(random.randint(0, 1000000))
    cluster = clusters.YT(proxy="hahn.yt.yandex.net") \
        .env(templates=dict(
            job_root=('tmp')))
    job = cluster.job()
    urls = job.table(input_table) \
              .map(get_music_tasks)
    if data_table:
        data = job.table(data_table) \
                  .join(urls, by='hyp') \
                  .put(output_table)
    else:
        urls.put(output_table)
    job.run()

    return {"cluster": "hahn", "table": output_table}


if __name__ == '__main__':
    call_as_operation(main)