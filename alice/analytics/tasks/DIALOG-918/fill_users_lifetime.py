#!/usr/bin/env python
# encoding: utf-8

from nile.api.v1 import clusters, filters
import nile.api.v1.aggregators as aggr
import yt.wrapper as yt

test_date = '{2017-04-21..2017-10-22}'

hahn = clusters.Hahn()

job = hahn.job()

inp_table = job.table('//home/voice/vins/logs/dialogs/%s' % test_date)

#out_path = '//home/voice/yoschi/tmp/daily_users_%s' % '13-15'  # % test_date
out_path = '//home/voice/yoschi/dialogs/users_lifetime'


client = yt.YtClient(proxy='hahn', config={"tabular_data_format": "tsv"})


schema = [{'name': 'uuid', "type" : "string"},
          {'name': 'first_time', "type" : "uint64"},
          {'name': 'last_time', "type" : "uint64"}]


def make_table(tname, schema):
    if client.exists(tname):
        pass #client.alter_table(tname, schema=schema)
    else:
        client.create_table(tname, attributes={'schema': schema})


make_table(out_path, schema)


out_table = inp_table.project('uuid', 'server_time'
                            ).groupby('uuid').aggregate(first_time=aggr.min('server_time'),
                                                        last_time=aggr.max('server_time'))

out = out_table.put(out_path)

job.run()

