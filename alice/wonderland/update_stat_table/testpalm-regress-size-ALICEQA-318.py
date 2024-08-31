#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import json
import itertools as it
from datetime import datetime, timedelta

import requests
from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util.retry import Retry

import sys
reload(sys)
sys.setdefaultencoding('utf-8')

###########################
# блок констант и токенов #

ST_API_URL = 'https://st-api.yandex-team.ru/v2/issues'
ST_TOKEN = os.getenv("STARTREK_OAUTH_TOKEN")
ST_HEADERS = {'Authorization': 'OAuth %s' % (ST_TOKEN)}

CHARTS_TOKEN = os.getenv("CHARTS_TOKEN")
SF_API_URL = 'https://upload.stat.yandex-team.ru/'
SF_HEADERS = {'Authorization': 'OAuth %s' % (ST_TOKEN)}
PATH = 'Adhoc/cubovaya/Graphs/case_count_without_uuid'

FILTER = '(Queue: ALICERELEASE OR Queue: SEAREL AND Components: "! quasar" OR Queue: SEAREL AND Components: "! iot" OR Queue: POODYA OR Queue: VIDEOBASSREL) AND Updated:>=today()-180d'
DICTIONARY = 'alice_components_for_dash'

TESTPALM_URL = 'https://testpalm-api.yandex-team.ru/testrun/'
TESTPALM_OAUTH_TOKEN = os.getenv("TESTPALM_OAUTH_TOKEN")
TESTPALM_CONTENT_HEADERS = {
    'Authorization': 'OAuth %s' % (TESTPALM_OAUTH_TOKEN),
    "Content-Type": "application/json"
}

#################
# testpalm part #
#################

# тестпалм пятисотит, ретраим. стат тоже лагает, но об POST надо подумать
def requests_retry_session(
    retries=3,
    backoff_factor=0.3,
    status_forcelist=(500, 502, 504),
    session=None,
):
    session = session or requests.Session()
    retry = Retry(
        total=retries,
        read=retries,
        connect=retries,
        backoff_factor=backoff_factor,
        status_forcelist=status_forcelist,
    )
    adapter = HTTPAdapter(max_retries=retry)
    session.mount('http://', adapter)
    session.mount('https://', adapter)
    return session


def find_testrun_id_and_append_to_list(remotelinks):
    testrun_id_list = []
    for i in remotelinks:
        display = i['display']
        start = display.find('testrun/alice/')
        start_len = start + len('testrun/')
        end = display.find('(ru.yandex.testpalm)')
        if start == -1:
            start = display.find('testrun/paskills/')
            start_len = start + len('testrun/')
            end = display.find('(ru.yandex.testpalm)')
        if start == -1:
            start = display.find('testrun/aliceui/')
            start_len = start + len('testrun/')
            end = display.find('(ru.yandex.testpalm)')
        if start == -1:
            start = display.find('testrun/iot-ui/')
            start_len = start + len('testrun/')
            end = display.find('(ru.yandex.testpalm)')
        if start == -1:
            start = display.find('testrun/quasar-ui/')
            start_len = start + len('testrun/')
            end = display.find('(ru.yandex.testpalm)')
        if start != -1:
            run_id = display[start_len:end]
            testrun_id_list.append(run_id)
    return testrun_id_list


def get_testrun_info(run_id):
    resp = requests_retry_session().get('%s/%s' % (TESTPALM_URL, run_id),
        params={'include': 'parentIssue.id,resolution.counter,duration'},
        headers=TESTPALM_CONTENT_HEADERS)
    if resp.status_code == 200:
        return resp.json()


def calculate_completed_cases_in_testrun(info):
    print(info)
    total = info['resolution']['counter']['total']
    skipped = info['resolution']['counter']['skipped']
    created = info['resolution']['counter']['created']
    return total-skipped-created

def testpalm_statistics(testrun_id_list):
    count = 0
    for i in testrun_id_list:
        info = get_testrun_info(i)
        if not info:
            continue
        count += calculate_completed_cases_in_testrun(info)
    return count


#####################
# create json value #
#####################

def get_tickets():
    page, page_list_len = 1, -1
    list_tickets = []

    while page_list_len != 0:
        resp = requests.get(ST_API_URL,
            params={
                'query': FILTER,
                'expand': 'links',
                'fields': 'tags,components,key,summary,created',
                'page': page},
            headers=ST_HEADERS)

        page_list_len = len(resp.json())
        print(page, page_list_len)

        if resp.json(): list_tickets.extend(resp.json())
        page += 1
    return list_tickets


def get_changelog(key):
    assert isinstance(key, str)
    resp = requests.get('%s/%s/changelog?field=status' \
        % (ST_API_URL, key), headers=ST_HEADERS)
    changelog = resp.json()
    return changelog


def add_3_hours(eventdate):
    eventdate = eventdate[:19]
    eventdate = eventdate.replace('T', ' ')
    eventdate = datetime.strptime(eventdate, "%Y-%m-%d %H:%M:%S")
    eventdate += timedelta(hours=3)
    return str(eventdate)


def find_components(body, tags_list):
    components = []
    if body['tags']:
        components = list( set(body['tags']) & set(tags_list) )
    return components


def get_tags_list():
    resp = requests.get('%s/dictionary/' % (SF_API_URL),
                        params={'name': DICTIONARY},
                        headers={'Authorization': 'OAuth %s' % (CHARTS_TOKEN)})
    print(resp.json())
    tags_list = resp.json()['tags_list']
    return tags_list


def create_base_dict(body):
    key = str(body['key'])
    changelog = get_changelog(key)
    date = add_3_hours(body['createdAt'])
    date = date[:10]

    body_dict = {
        "key": key,
        "summary": body['summary'],
        "fielddate" : date,
        }
    return body_dict


def calculate_case_count(body):
    size_of_all_testruns = 0
    remotelinks = body.get('remotelinks', 0)
    if remotelinks:
        testrun_id_list = find_testrun_id_and_append_to_list(remotelinks)
        size_of_all_testruns = testpalm_statistics(testrun_id_list)
    return size_of_all_testruns


def create_result():
    list_tickets = get_tickets()
    res = []
    tags_list = get_tags_list()
    for ticket_body in list_tickets:
        print(ticket_body['key'])
        components = find_components(ticket_body, tags_list)
        for component in components:
            ticket_dict = create_base_dict(ticket_body)
            ticket_dict["component"] = component
            ticket_dict["case_count"] = calculate_case_count(ticket_body)
            res.append(ticket_dict)
    res.sort(key = lambda n: n['fielddate'])
    res = list(filter(lambda a: a["case_count"] != 0, res))
    return res

####################
# post to statface #
# ####################

def post_statface(res):
    assert isinstance(res, list)
    resp = requests.post('%s/_api/report/data/%s/' % (SF_API_URL, PATH),
                        params={'scale': 'daily', 'replace_mask': 'fielddate'},
                        headers={'Authorization': 'OAuth %s' % (CHARTS_TOKEN)},
                        json={'data': res})
    print(resp.status_code, resp.json())
    try:
        return resp.json()['message']
    except KeyError:
        print('Не удалось загрузить отчёт\n' + str(resp.json()))

res = create_result()
print(res)
post_statface(res)
