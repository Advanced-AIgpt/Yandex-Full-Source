#-*- coding: utf-8 -*-

import json
import os
import requests
from datetime import datetime, timedelta

TESTPALM_TESTCASES_URL = 'https://testpalm.yandex-team.ru/api/testcases/elari'
TESTPALM_EVENTSLOG_URL = 'https://testpalm.yandex-team.ru/api/eventslog/elari'
TESTPALM_SUITE_URL = 'https://testpalm.yandex-team.ru:443/api/testsuite/elari'
TESTPALM_VERSION_URL = 'https://testpalm.yandex-team.ru:443/api/version/elari'

TESTPALM_OAUTH_TOKEN = os.getenv("TESTPALM_OAUTH_TOKEN")
TESTPALM_HEADER = {'Authorization': 'OAuth %s' % (TESTPALM_OAUTH_TOKEN)}
TESTPALM_CONTENT_HEADERS = {
    'Authorization': 'OAuth %s' % (TESTPALM_OAUTH_TOKEN),
    "Content-Type": "application/json"
}

TESTING_KEY_ID = '5d7650e63d42cbea927d5705' # testing
SURFACE_KEY_ID = '5d766201d284f917bffc781d' # surface
SCENATIO_KEY_ID = '5d76511646af841bf0c1e2bd' # scenario

def add_one_key(type_add,attribute,value):
    return {
            'type': type_add,
            'key': attribute,
            'value': value
        }

# делаем фильтр по списку тегов
def filter(keys):
    new_filter = {}
    # assessors
    if len(keys) == 1:
        new_filter = add_one_key('EQ', 'attributes.' + TESTING_KEY_ID, 'assessors')
    # assessors, surf
    if len(keys) == 2:
        new_filter = {'type':'AND','left':{'type':'EQ','key':'attributes.'+TESTING_KEY_ID,'value':'assessors'},'right':{'type':'EQ','key':'attributes.'+SURFACE_KEY_ID,'value':keys[1]}}
    # assessors, surf, scenario
    # проблема с кейсами без компоненты (?)
    if len(keys) == 3:
        if keys[2] == 'Без компоненты': keys[2] = 'null'
        new_filter = {'type':'AND','left':{'type':'AND','left':{'type':'EQ','key':'attributes.'+TESTING_KEY_ID,'value':'assessors'},'right':{'type':'EQ','key':'attributes.'+SURFACE_KEY_ID,'value':keys[1]}},'right':{'type':'EQ','key':'attributes.'+SCENATIO_KEY_ID,'value':keys[2]}}
    return new_filter

# все кейсы по фильтру
def get_cases(surf):
    new_filter = filter(['assessors', surf])
    new_filter = json.dumps(new_filter, ensure_ascii=False, separators=(",", ": "))
    case_resp = requests.get(TESTPALM_TESTCASES_URL, params={'expression': new_filter}, headers=TESTPALM_CONTENT_HEADERS)
    if not case_resp.json():
        print(new_filter)
        print('Не удалось достать кейсы')
        return []
    else:
        return case_resp.json()

def create_suite(surf, suite_filter, s):
    date = str((datetime.now()).strftime('%Y-%m-%d'))
    body = json.dumps({'tags': ['notSuitableForFarm'], 'title': 'Алиса ' + surf + ' ' + date + ' ' + s, 'filter': {'expression': suite_filter}})
    body = body.encode('utf-8')
    resp = requests.post(TESTPALM_SUITE_URL, data=body, headers=TESTPALM_CONTENT_HEADERS)
    #print (resp, resp.text)
    resp = json.loads(resp.text)
    return resp

def create_version(ids):
    date = str((datetime.now()).strftime('%Y-%m-%d'))
    i = date.replace('-', '')
    body = json.dumps({'title': 'Алиса ' + date, 'id': 'Alice' + i, 'suites' : ids})
    print (body)
    body = body.encode('utf-8')
    resp = requests.post(TESTPALM_VERSION_URL, data=body, headers=TESTPALM_CONTENT_HEADERS)
    print (resp, resp.text)
    return resp.text


def version(key):
    if not key in ['all', 'searchapp', 'station', 'navi', 'loudspeaker', 'launcher', 'myabro', 'module', 'station mini', 'yabro_win']:
        print ('Неправильный ключ')
        return []

    surfaces = []
    if key == 'all':
        surfaces =  ['searchapp', 'station', 'navi', 'loudspeaker', 'launcher', 'myabro', 'module', 'station mini', 'yabro_win']
    else:
        surfaces.append(key)

    for surf in surfaces:
        # создаём фильтр по поверхности, по которой запускаем
        cases_filter = filter(['assessors', surf])

        # посмотрим, какие компоненты есть
        cases = get_cases (surf)
        if not cases:
            print(surf, len(cases))
        else:
            # если делать по сценариям
            scenarios = {}
            for elem in cases:
                if SCENATIO_KEY_ID in elem['attributes']:
                    s = elem['attributes'][SCENATIO_KEY_ID][0]
                else: s = 'Без компоненты'
                if s in scenarios:
                    scenarios[s] += 1
                else:
                    scenarios[s] = 0
            #print(scenarios)
            ids = []
            for s in scenarios:
                filter_for_suite = filter(['assessors', surf, s])
                resp = create_suite(surf, filter_for_suite, s)
                if not resp:
                    print (surf, filter_for_suite, s)
                    print ('Не удалось создать сьюты')
                else: ids.append(resp['id'])
    #print(ids)
    resp = create_version(ids)
    if not resp:
        print ('Не удалось создать версию')
    return

# пример:
version('all')
