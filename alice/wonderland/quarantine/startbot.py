#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import json
import datetime
import time
import requests

###########################
# блок констант и токенов #
###########################

TESTPALM_TESTCASES_URL = 'https://testpalm.yandex-team.ru/api/testcases/alice'
TESTPALM_EVENTSLOG_URL = 'https://testpalm.yandex-team.ru/api/eventslog/alice'
TESTING_KEY_ID = '5bb1136a798633db7f1b5dd2'

TESTPALM_OAUTH_TOKEN = os.getenv("TESTPALM_OAUTH_TOKEN")
TESTPALM_HEADER = {'Authorization': 'OAuth %s' % (TESTPALM_OAUTH_TOKEN)}
TESTPALM_CONTENT_HEADERS = {
    'Authorization': 'OAuth %s' % (TESTPALM_OAUTH_TOKEN),
    "Content-Type": "application/json"
}

STARTREK_API_URL = 'https://st-api.yandex-team.ru/v2/issues'
STARTREK_ROBOT_TOKEN = os.getenv("STARTREK_OAUTH_TOKEN")
STARTREK_ROBOT_HEADER = {'Authorization': 'OAuth %s' % (STARTREK_ROBOT_TOKEN)}

YAMB_API_URL = 'https://bp.mssngr.yandex.net/bot'
YAMB_TOKEN = os.getenv("YAMB_TOKEN")
YAMB_CHAT_ID = os.getenv("YAMB_CHAT_ID")
YAMB_HEADER = {'Authorization': 'OAuthTeam %s' % (YAMB_TOKEN)}

#################################
# блок общих несценарных функций#
#################################

def write_logs(string):
    global log_comment
    log_comment += string + '\n'
    return log_comment

def post_yamb_message(log_comment):
    assert isinstance(log_comment, str)
    message = {"chat_id": YAMB_CHAT_ID, "text": log_comment}
    print(message)
    resp = requests.post( YAMB_API_URL + '/sendMessage/',
                        data=message,
                        headers=YAMB_HEADER)
    try:
        return resp.json()['message']
    except KeyError:
        print('Не удалось отправить комментарий\n' + str(resp.json()))
    print(resp.json())

def convert_date_n_days_ago_to_ms(days_count):
    assert isinstance(days_count, int)
    time_now = datetime.datetime.today()
    time_now = time_now.replace(hour=0, minute=0, second=0, microsecond=0)
    time_days_ago = time_now - datetime.timedelta(days=days_count)
    time_days_ago = int(time.mktime(time_days_ago.timetuple()) * 1000 + float(time_days_ago.microsecond) / 1000)
    return time_days_ago

def add_testing_key(type_add,attribute,value):
    return {
            'type': type_add,
            'key': attribute,
            'value': value
        }

def get_case_body(case_id):
    resp = requests.get('%s/?id=%d' % (TESTPALM_TESTCASES_URL, case_id),
                            headers=TESTPALM_HEADER)
    body_resp = resp.json()
    return body_resp


def put_case_body(body):
    assert isinstance(body, dict)
    filter_expression = body
    filter_expression = json.dumps(filter_expression, ensure_ascii=False)
    filter_expression = filter_expression.encode('utf-8')
    resp = requests.put(TESTPALM_TESTCASES_URL,
                         data=filter_expression,
                         headers=TESTPALM_CONTENT_HEADERS)
    try:
        return resp.json()['id']
    except KeyError:
        write_logs('Error while updating case\n' + {resp.json()})


def remove_duplicates_tikets_list(tikets_list):
    assert isinstance(tikets_list, list)
    result_list = []
    for i in tikets_list:
        if i not in result_list:
            result_list.append(i)
    return result_list

#######################################
# профит (найденные в карантине баги) #
#######################################

def get_quarantine_cases():
    filter_ = add_testing_key('EQ','attributes.' + TESTING_KEY_ID,'quarantine')
    filter_ = json.dumps(filter_, ensure_ascii=False, separators=(",", ": "))
    case_resp = requests.get(TESTPALM_TESTCASES_URL,
                        params={'expression': filter_},
                        headers=TESTPALM_CONTENT_HEADERS)
    if not case_resp.json():
        write_logs('В карантине ничего нет')
        return []
    else:
        yesterday_quarantine_cases = [c['id'] for c in case_resp.json()]
        write_logs('Профит собираем в кейсах: ' + str(yesterday_quarantine_cases))
        return yesterday_quarantine_cases

def get_case_bugs(case_id):
    assert isinstance(case_id, int)
    case_resp = requests.get('%s/?id=%d&includeFields=bugs' % (TESTPALM_TESTCASES_URL, case_id),
                        headers=TESTPALM_CONTENT_HEADERS)
    case_bugs = case_resp.json()[0]['bugs']
    if not case_bugs:
        return []
    else:
        return [c['id'] for c in case_bugs]

def get_case_tasks(case_id):
    assert isinstance(case_id, int)
    case_resp = requests.get('%s/?id=%d&includeFields=tasks' % (TESTPALM_TESTCASES_URL, case_id),
                        headers=TESTPALM_HEADER)
    case_tasks = case_resp.json()[0]['tasks']
    if not case_tasks:
        return []
    else:
        return [c['id'] for c in case_tasks]

def check_creation_date_and_queue(ticket_id):
    assert isinstance(ticket_id, str)
    query = 'Queue%3A%20ALICE%2CALICEASSESSORS%20AND%20Created%3A%20>%3D%20today()%20-2d%20AND%20Key%3A%20' + str(ticket_id)
    resp = requests.get('%s/?query=%s' % (STARTREK_API_URL, query),
                        headers=STARTREK_ROBOT_HEADER)
    if resp.json():
        return 'ok'

def add_quarantine_tag_on_ticket(ticket_id):
    assert isinstance(ticket_id, str)
    add_tag = json.dumps({"tags":{"add":["quarantine"]}}, ensure_ascii=False)
    resp = requests.patch('%s/%s' % (STARTREK_API_URL, ticket_id),
                         data=add_tag,
                         headers=STARTREK_ROBOT_HEADER)
    try:
        return resp.json()['key']
    except KeyError:
        write_logs('Error while updating https://st.yandex-team.ru/' + ticket_id + '\n' + str(resp.json()))

def profit(quarantine_cases_list):
    assert isinstance(quarantine_cases_list, list)
    for case_id in quarantine_cases_list:
        quarantine_tickets_list = get_case_bugs(case_id) + get_case_tasks(case_id)
        quarantine_tickets_list = remove_duplicates_tikets_list(quarantine_tickets_list)
        print(quarantine_tickets_list)
        if quarantine_tickets_list:
            for ticket_id in quarantine_tickets_list:
                ticket_id = str(ticket_id)
                if check_creation_date_and_queue(ticket_id):
                    add_quarantine_tag_on_ticket(ticket_id)
                    write_logs('https://st.yandex-team.ru/' + ticket_id + ' go to quarantine')



######################
# вывод из карантина #
######################

def get_tag_added_time(case_id):
    assert isinstance(case_id, int)
    filter_ = add_testing_key('CONTAIN','changes.changedAttributes.' + TESTING_KEY_ID + '.added','quarantine')
    filter_ = json.dumps(filter_, ensure_ascii=False, separators=(",", ": "))
    event_resp = requests.get(TESTPALM_EVENTSLOG_URL,
                    params={'expression': filter_, 'testcaseId': case_id},
                    headers=TESTPALM_HEADER)
    if not event_resp.json():
        return[]
    else:
        return event_resp.json()[0]['lastModifiedTime']

def get_creation_time(case_id):
    assert isinstance(case_id, int)
    event_resp = requests.get(TESTPALM_EVENTSLOG_URL,
                    params={'type': 'CREATED', 'testcaseId': case_id},
                    headers=TESTPALM_HEADER)
    if not event_resp.json():
        return[]
    else:
        return event_resp.json()[0]['createdTime']

def remove_quarantine_tag(body_resp):
    assert isinstance(body_resp, list)
    body_resp[0]['attributes'][TESTING_KEY_ID].remove('quarantine')
    return body_resp

def remove_from_quarantine(case_id):
    assert isinstance(case_id, int)
    stage = get_case_body(case_id)
    stage = remove_quarantine_tag(stage)
    stage = put_case_body(stage[0])

def check_quarantine_cases(cases_id):
    assert isinstance(cases_id, list)
    for case_id in cases_id:
        if get_tag_added_time(case_id):
            compare_time = get_tag_added_time(case_id)
        elif get_creation_time(case_id):
            compare_time = get_creation_time(case_id)
        two_week_ago_time = convert_date_n_days_ago_to_ms(13)
        if compare_time < two_week_ago_time:
            remove_from_quarantine(case_id)
            write_logs('Кейс ' + str(case_id) + ' выходит из карантина')

###################
# вход в карантин #
###################

def get_new_no_smoke_cases():
    ms_time = convert_date_n_days_ago_to_ms(1)
    filter_ = add_testing_key('NEQ','attributes.' + TESTING_KEY_ID,'smoke')
    filter_ = json.dumps(filter_, ensure_ascii=False, separators=(",", ": "))
    case_resp = requests.get(TESTPALM_TESTCASES_URL,
                        params={'expression': filter_,'from_createdTime': ms_time},
                        headers=TESTPALM_HEADER)
    if not case_resp.json():
        write_logs('Сегодня новых кейсов не писали')
        return []
    else:
        return [c['id'] for c in case_resp.json()]

def get_bugs_from_testpalm_project():
    bugs_list = []
    bugs_resp = requests.get('%s/?includeFields=bugs.id' % (TESTPALM_TESTCASES_URL),
                        headers=TESTPALM_HEADER)
    if not bugs_resp.json():
        write_logs('Нет багов в проекте (странно, не так ли?)')
        return []
    else:
        for c in bugs_resp.json():
            if c["bugs"] != []:
                for d in c["bugs"]:
                    bugs_list.append(d['id'])
    return bugs_list

def get_tasks_from_testpalm_project():
    tasks_list = []
    tasks_resp = requests.get('%s/?includeFields=tasks.id' % (TESTPALM_TESTCASES_URL),
                        headers=TESTPALM_HEADER)
    if not tasks_resp.json():
        write_logs('Поле tasks везде пустое (бывает)')
        return []
    else:
        for c in tasks_resp.json():
            if c["tasks"] != []:
                for d in c["tasks"]:
                    tasks_list.append(d['id'])
    return tasks_list

def get_remotelinks_list(result_list):
    assert isinstance(result_list, list)
    remotelinks_list = []
    for ticket_key in result_list:
        query = 'Resolved%3A%20>%3D%20today()%20-1d%20AND%20Resolution%3A%20Решен%20AND%20Key%3A%20' + str(ticket_key)
        resp = requests.get(STARTREK_API_URL + '/?expand=links&fields=key&query=' + query,
                            headers=STARTREK_ROBOT_HEADER)
        if resp.json():
            for d in resp.json()[0]['remotelinks']:
                remotelinks_list.append(d['display'])
    return remotelinks_list

def get_cases_with_solved_bugs(remotelinks_list):
    case_list = []
    for c in remotelinks_list:
        start = c.find('testcases/alice/')
        start_len = start + len('testcases/alice/')
        end = c.find('(ru.yandex.testpalm)')
        if start != -1:
            case_id = int(c[start_len:end])
            if check_case_for_smoke(case_id):
                case_list.append(case_id)
    return case_list

def check_case_for_smoke(case_id):
    filter_ = add_testing_key('NEQ','attributes.' + TESTING_KEY_ID,'smoke')
    filter_ = json.dumps(filter_, ensure_ascii=False, separators=(",", ": "))
    smoke_resp = requests.get(TESTPALM_TESTCASES_URL,
                        params={'expression': filter_,"id": case_id},
                        headers=TESTPALM_CONTENT_HEADERS)
    if smoke_resp.json():
        return 'ok'

def add_quarantine_tag_on_case_body(body_resp):
    assert isinstance(body_resp, list)
    body_resp[0]['attributes'].setdefault(TESTING_KEY_ID, ['quarantine'])
    body_resp[0]['attributes'][TESTING_KEY_ID].append('quarantine')
    return body_resp

def add_quarantine_tag_on_case(quarantine_pretendent_list):
    assert isinstance(quarantine_pretendent_list, list)
    for case_id in quarantine_pretendent_list:
        stage = get_case_body(case_id)
        stage = add_quarantine_tag_on_case_body(stage)
        stage = put_case_body(stage[0])

def go_cases_to_quarantine():
    tikets_list = get_bugs_from_testpalm_project() + get_tasks_from_testpalm_project()
    tikets_list_without_doubles = remove_duplicates_tikets_list(tikets_list)
    remotelinks_list = get_remotelinks_list(tikets_list_without_doubles)
    quarantine_pretendent_list = get_cases_with_solved_bugs(remotelinks_list) + get_new_no_smoke_cases()
    if len(quarantine_pretendent_list) != 0:
        write_logs('В карантин заходят кейсы: ' + str(quarantine_pretendent_list))
        add_quarantine_tag_on_case(quarantine_pretendent_list)
    else:
        write_logs('Нет кейсов, попадающих в карантин.')

log_comment = 'Запускаю карантин\n'

quarantine_cases_list = get_quarantine_cases()
profit(quarantine_cases_list)
check_quarantine_cases(quarantine_cases_list)
go_cases_to_quarantine()

post_yamb_message(log_comment)
