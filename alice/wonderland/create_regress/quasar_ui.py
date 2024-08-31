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
    #        print('Can not add run into ticket')
    #        return []
    return run

def get_suites(release):
    if not release in ['test', 'quasar_ui_smoke', 'quasar_ui_full', 'smarthome']:
        print ('Wrong release tag')
        return []

    params = { "include": "id,title,tags", "expression": '''{{"type": "EQ", "key": "tags", "value": "{tag}" }}'''.format(tag=release)}
    resp = requests.get(alice_suite_url, headers=testpalm_header, params=params, verify=False)
    print(resp.text)
    if not resp.text: # не получили сьюты
        return []
    else:
        return resp.json()

def create_assessors_ticket(release, ticket, runs):
    runs_in_ticket = ''
    for el in runs:
        runs_in_ticket += el + '\n'
    description = "==Релиз " + release + '\n\n' + "Релизный тикет " + ticket + '\n\n' + '==Настройки окружения' + '\n\n' 
    description +=  '===1. **Залипнуть в тестид 268705 и убедиться, что отображается ((https://testpalm.yandex-team.ru/testcase/alice-2762 номер версии))**\n\n'
    

    description +=  '<{Если не получилось\n\n'
    description +=  '#|' + '\n' + '|| **ПП**| %%yellowskin://?url=https%3A%2F%2Frctemplates-shared.hamster.yandex.ru%2Fquasar%3Fsrcrwr%3DQUASAR_HOST%253Aquasar.yandex.ru%26srcrwr%3DBILLING_HOST%253Aquasar.yandex.ru%26srcrwr%3DIOT_HOST%253Aiot.quasar.yandex.ru%%||' + '\n' + '|| **Десктоп**| https://rctemplates-shared.hamster.yandex.ru/quasar?srcrwr=QUASAR_HOST%3Aquasar.yandex.ru&srcrwr=BILLING_HOST%3Aquasar.yandex.ru&srcrwr=IOT_HOST%3Aiot.quasar.yandex.ru||' + '\n' + '|#'
    description +=  '\n\n' + 'QR код для пп:' + '\n' + '0x0:http://qr2.yandex.net/?text=yellowskin%3A%2F%2F%3Furl%3Dhttps%253A%252F%252Frctemplates-shared.hamster.yandex.ru%252Fquasar%253Fsrcrwr%253DQUASAR_HOST%25253Aquasar.yandex.ru%2526srcrwr%253DBILLING_HOST%25253Aquasar.yandex.ru%2526srcrwr%253DIOT_HOST%25253Aiot.quasar.yandex.ru'
    description +=  '}>'

    description +=  '\n\n' + "===2. Тест-ран Моя Алиса тестировать в !!табах Алисы!! (открыть диалог с Алисой по тапу на окникс и перейти на вкладку Устройства). Остальные два тест-рана - на вкладке Устройства из боковой панели\n\n" 
    description +=  '\n\n' + "Настройки для тестирования Моей Алисы:\n\n" + "- в разделе Flags нужно включить %%enableAliceTabsBar = true, enableAliceOknyxOverTabs = true%%\n\n" 
    
    description +=  '- настроить табы'
    description +=  '<{Если смог залипнуть в тестид\n\n'
    description +=  '\n\n' + 'в AliceTabsConfiguration вставить {"home" : {"uri" : "https://yandex.ru/alice/home", "order": 0},"dialog" : {"order" : 1},"skills" : {"uri" : "https://dialogs.yandex.ru/store", "order": 2},"devices" : {"uri" : "https://yandex.ru/quasar/iot", "order": 3},"settings" : {"uri" : "https://yandex.ru/quasar/account", "order": 4} }'
    description +=  '}>' 

    description +=  '<{Если НЕ смог залипнуть в тестид\n\n'
    description +=  '\n\n' + 'в AliceTabsConfiguration вставить {"home" : {"uri" : "https://yandex.ru/alice/home", "order": 0},"dialog" : {"order" : 1},"skills" : {"uri" : "https://dialogs.yandex.ru/store", "order": 2},"devices" : {"uri" : "https://rctemplates-shared.hamster.yandex.ru/quasar/iot", "order": 3},"settings" : {"uri" : "https://rctemplates-shared.hamster.yandex.ru/quasar/account", "order": 4} }'
    description +=  '}>' 

    description += '\n\n' + "==Инструкция по тестированию\nhttps://wiki.yandex-team.ru/alicetesting/assessors-and-alice/"
    description +=  '\n\n' + "==Список ранов\n\n" + '<[' + runs_in_ticket + ']>'
    data = {"queue": "ALICEASSESSORS", "type": "task", "summary": "Тестирование релиза " + release + " " + ticket, "description": description, "parent": ticket}
    raw_data = json.dumps(data, ensure_ascii=False, separators=(",", ": "))
    raw_data = raw_data.encode('utf-8')

    resp = requests.post(st_api_url, data=raw_data, headers=st_header)

    if not resp.json():
        return []
    return resp

def create_cases_ticket(ticket):
    data = {"queue": "ALICE", "type": "task", "summary": "Кейсы по релизу UI", "description": ticket, "assignee":{"id":"osennikovak"}, "parent": ticket, "tags": 'cases'}
    raw_data = json.dumps(data, ensure_ascii=False, separators=(",", ": "))
    raw_data = raw_data.encode('utf-8')
    resp = requests.post(st_api_url, data=raw_data, headers=st_header)
    return resp, resp.text

def add_tag(release, ticket):
    data = {"tags": { "add": release }}
    raw_data = json.dumps(data, ensure_ascii=False, separators=(",", ": "))
    raw_data = raw_data.encode('utf-8')
    resp = requests.patch(st_api_url + ticket, data=raw_data, headers=st_header)
    print (resp, resp.text)
    return resp, resp.text

# первый параметр - релиз, варианты: quasar_ui_smoke, quasar_ui_full
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
            title = item["title"]
            tags = item["tags"]
            suite_id = item["id"]
            run = create_run(suite_id, title, tags, ticket)
            if not run:
                print ('Can not create run ' + title)
            else:
                run_id = run['id']
                list_runs_for_assessors.append(item["title"] + ' ' + 'https://testpalm.yandex-team.ru/alice/testrun/' + str(run_id))
    add_tag(release, ticket)
    create_assessors_ticket(release, ticket, list_runs_for_assessors)
    create_cases_ticket(ticket)
    return

# как запустить локально:
create_regress ('quasar_ui_smoke', 'SEAREL-16985')
