#!/usr/bin/env python
from nile.api.v1 import (
    clusters,
    Record,
    filters as nf
)
import yt.wrapper as yt
import re

from utils.nirvana.op_caller import call_as_operation


def main(annotation_table="//home/voice/toloka/ru-RU/daily/carheadunit/2018-09-02__2018-09-09",
         recs = 1500,
         car_models = ["Allwinner t3-", "Android px3-rio-microntek_n-yaCS", "TW Captur-astar-yaCS", "rockchip d200"]):
    cluster = clusters.YT(proxy="hahn.yt.yandex.net")

    job = cluster.job()
    yt.config.set_proxy("hahn")

    # 20 is from a desire not to join all the logs that we encountered in good_tables, but still have enought to satisfy number of recs (for 4000 recs i observed nessecity for ~10 good_tables tables).
    logs = job.table('//home/voice-speechbase/uniproxy/logs_v3/{2018-09-02..2018-09-09}')
    logs = logs.filter(nf.equals('lang', 'ru-RU'),
                       nf.equals('key', '1a198b89-2443-4ac9-85b9-9db178271aec'),
                       nf.equals('requestTopic', 'autolauncher'))  # discard all irrelevant logs right-away
    logs = logs.project('mds_key',
                        'device')  # 'device' is the entire reason we need to look in the logs again (all other info already in tables wit annotations)

    annotations = job.table(annotation_table)
    # annotations = annotations.filter(nf.equals('mark', 'TEST'))
    annotations = annotations.join(logs, by='mds_key')

    paths = {}

    for model in car_models:
        path = yt.create_temp_table('//home/voice/nikitachizhov/tmp', expiration_timeout=1000*60*60*3)

        annotations \
            .filter(nf.custom(lambda device, m=model: device.startswith(m), 'device')) \
            .top(recs, by='ts') \
            .project("mark", "text", "url", "voices") \
            .put(path[:])

        paths[model] = path[:]

    job.run()

    out = {}

    for model in car_models:
        out[model] = [row for row in yt.read_table(paths[model], format=yt.format.JsonFormat())]

    return out

if __name__ == '__main__':
    call_as_operation(main)