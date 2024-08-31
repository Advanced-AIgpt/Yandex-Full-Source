#!/usr/bin/env python
# encoding: utf-8
from os.path import join, realpath

from utils.auth import choose_credential

# Явки для запросов в наш кластер DBaaS


def make_headers(username='viewer', password=None):
    return {'X-ClickHouse-User': username,
            'X-ClickHouse-Key': choose_credential(password,
                                                  'CH_VIEWER_PASSWD',
                                                  '~/.dbaas/viewer_passwd')}


# Наши хосты в DBaaS
# $ yc mdb cluster ListHosts --clusterId fb3a2ad2-d435-45ee-af23-13b55c441df5
HOSTS = ['vla-fpfbc2lbf333m9bf.db.yandex.net',  # Хост, на который регулярно заливаются логи по сессиям.
                                                # Он же добавлен  В YQL  в качесте кластера voicelog
         'man-5wu7zgkymt64fksp.db.yandex.net',  # Для экспериментов. Если хочется с ним поработать,
                                                # можно просто закомментить предыдущий,
                                                #  или передавать хост явно в соответствующие функции (post_req, insert_req)
         ]

