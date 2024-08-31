#-*- coding: utf-8 -*-

import json
import requests
import datetime
from requests.packages.urllib3.util.retry import Retry
from requests.adapters import HTTPAdapter

testpalm_token = '' # личный токен или робота
st_token = '' # личный токен или робота

testpalm_header = {
    'Authorization': f'OAuth {testpalm_token}',
    "Content-Type": "application/json"}

st_header = {'Authorization': f'OAuth {st_token}',
    "Content-Type": "application/json"}


TESTPALM_API_URL = "https://testpalm-api.yandex-team.ru"
TESTPALM_URL = "https://testpalm.yandex-team.ru"

#alice_url = 'https://testpalm.yandex-team.ru:443/api/testcases/elari'
alice_run_suite_url = TESTPALM_API_URL + '/testrun/alice/create/'
alice_suite_url = TESTPALM_API_URL + '/testsuite/alice/'


st_api_url = 'https://st-api.yandex-team.ru/v2/issues/'


#ticket = QUEUE-N, run - вида '5c00574c798633871cb92380' -- только номер от /alice/testrun/5c00574c798633871cb92380
def add_testrun_to_st (ticket, run):
    link_run = 'testrun/alice/' + run
    data = {"relationship": "relates","key": link_run, "origin": "ru.yandex.testpalm"}
    raw_data = json.dumps(data, ensure_ascii=False, separators=(",", ": "))
    raw_data = raw_data.encode('utf-8')

    resp = requests.post(st_api_url + ticket + '/remotelinks?notifyAuthor=false&backlink=false',
                        data=raw_data,
                        headers=st_header)
    if not resp.json():
        return []
    return resp

# title, suite_id - строка, tags - список строк, queue, num - строки 'QUEUE-NUM'
def create_run(suite_id, title, tags, ticket):
    parentIssue = {"id": ticket, "trackerId": "Startrek"}

    url = alice_run_suite_url

    params = { "include": "id,title",}
    payload = {
        "title": title,
        "testSuite": {
            "id": suite_id
        },
        "parentIssue": parentIssue,
    }
    response = requests.post(url, json=payload, params=params, headers=testpalm_header, verify=False)
    if response.status_code != 200:
        print("Unable to create testpalm run, code: {code}, body: {body}".format(code=response.status_code, body=response.text))
        return
    run = next(iter(response.json()), {})
    run_id = run['id']
    #if not add_testrun_to_st(ticket, run_id):
    #        print('Can not add rin into ticket')
    #        return []
    return run

def get_suites(release):
    if not release in ['test', 'VINS', 'BASS', 'quasar', 'uniproxy', 'billing', 'irbis', 'dexp', 'chetverg', 'smarthome', 'megamind', 'station mini', 'hollywood']:
        print ('Wrong release tag')
        return []

    params = { "include": "id,title,tags", "expression": '''{{"type": "EQ", "key": "tags", "value": "{tag}" }}'''.format(tag=release)}
    resp = requests.get(alice_suite_url, headers=testpalm_header, params=params, verify=False)
    if not resp.text: # не получили сьюты
        return []
    else:
        return resp.json()

def create_assessors_ticket(release, ticket, runs):
    runs_in_ticket = ''
    for el in runs:
        runs_in_ticket += el + '\n'
    description = "Релиз " + release + '\n\n' + "Релизный тикет " + ticket + '\n\n' + 'Конфиг: ' + '\n\n' + "Список ранов:\n\n" + '<[' + runs_in_ticket + ']>'
    description += '\n\n' + "Инструкция по тестированию https://wiki.yandex-team.ru/alicetesting/assessors-and-alice/"
    data = {"queue": "ALICEASSESSORS", "type": "task", "summary": "Тестирование релиза " + release + " " + ticket, "description": description, "parent": ticket}
    raw_data = json.dumps(data, ensure_ascii=False, separators=(",", ": "))
    raw_data = raw_data.encode('utf-8')

    resp = requests.post(st_api_url, data=raw_data, headers=st_header)

    if not resp.json():
        return []
    return resp

# первый параметр - релиз, варианты: VINS, BASS, quasar, uniproxy, billing, irbis, dexp, chetverg
# release, ticket - строки
def create_regress(release, ticket):
    list_runs_for_assessors = []
    date = str(datetime.datetime.now())
    date = date[8] + date[9] + '.' + date[5] + date[6]

    default_title = ' ' + ticket + ': ' + release.upper() + ' ' + date

    suites = get_suites(release)

    if not suites:
        print ('Can not get suites by ' + release + ' tag')
    else:
        #print (suites)
        for item in suites:
            title = item["title"][:item["title"].find(']') + 1] + default_title # сорян, некрасивый костыль
            tags = item["tags"]
            suite_id = item["id"]
            run = create_run(suite_id, title, tags, ticket)
            if not run:
                print ('Can not create run ' + title)
            else:
                run_id = run['id']
                list_runs_for_assessors.append(item["title"][:item["title"].find(']') + 1] + ' ' + 'https://testpalm.yandex-team.ru/alice/testrun/' + str(run_id))
    create_assessors_ticket(release, ticket, list_runs_for_assessors)
    return

# как запустить локально:
create_regress ('uniproxy', 'ALICERELEASE-1')
