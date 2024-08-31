#!/usr/bin/env python
# encoding: utf-8

import sys
from nile.api.v1 import clusters
from qb2.api.v1 import filters as qf
from nile.api.v1 import filters as nf


date = '2017-10-18'  # '2017-10-13'
prefix = 'c2-e3' #'a0-b1'

src_path = '//home/voice/vins/logs/dialogs/%s' % date
trg_path = '//home/voice/yoschi/dialogs/%s-%s' % (date, prefix)
selector = 'uu/%s.*' % ''.join('[%s]' % p for p in prefix.split('-'))
prod_apps = {"ru.yandex.mobile", "ru.yandex.searchplugin"}

hahn = clusters.Hahn()

job = hahn.job()

src_table = job.table(src_path)


def is_prod_app(request, *args):
    try:
        if request is None:
            return False
        return request['app_info']['app_id'] in prod_apps
    except KeyError:
        return False
    except Exception, err:
        sys.stderr.write('err: %s, request: %s, type_request: %s' % (err, request, type(request)))
        raise

trg_table = src_table.filter(qf.match('uuid', selector), nf.custom(is_prod_app, "request"))


out = trg_table.put(trg_path)

job.run()

