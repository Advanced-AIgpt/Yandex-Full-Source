#!/bin/env python
# coding=utf-8

import sys
import json
import codecs
import urllib2

import requests

fixlist_in = sys.argv[1]
offset = int(sys.argv[2]) if len(sys.argv) > 2 else 1
print 'Start from line', offset


def test0(query, marker, client, os, title, app, url):
    print u'marker={}, client={}, os={}, title={}, app={}, url={}'.format(marker, client, os, title, app, url)
    request = {
        'form': {
            'name': 'personal_assistant.scenarios.open_site_or_app',
            'slots': [
                {
                    'name': 'target',
                    'optional': False,
                    'type': 'string',
                    'value': query
                },
                {
                    'name': 'target_type',
                    'optional': True,
                    'type': 'site_or_app',
                    'value': marker
                }
            ]
        },
        'meta': {
            'user_agent': 'Mozilla/5.0 (' + os + '; CPU OS 7_0 like Mac OS X) AppleWebKit/537.51.1 (KHTML, like Gecko) Version/7.0 Mobile/11A465 Safari/9537.53',
            'client_id': client + ' (LInux; ' + os + ' 5.1.1;)',
            'epoch': 1486036540,
            'location': {
                'lat': 55.665589,
                'lon': 37.561917
            },
            'tz': 'Europe/Moscow',
            'uuid': '276756316',
            'utterance': query
        }
    }
    response = requests.post('http://localhost:12345/vins', json=request)
    answer = json.loads(response.text, encoding='utf-8')

    assert answer['form']['name'] == 'personal_assistant.scenarios.open_site_or_app'

    has_result = False
    for slot in answer['form']['slots']:
        if slot['name'] == 'navigation_results':
            assert slot['value']['text'] == title
            has_result = True
            break
    assert has_result

    has_open_suggest = os == 'ios'  # don't check on iOS
    has_serp = False
    for block in answer['blocks']:
        if not has_open_suggest and block['type'] == 'suggest' and block['suggest_type'] == 'open_site_or_app__open':
            uri = block['data']['uri']
            if app != '' and os == 'android':
                if marker == 'app' or url == '':
                    fb = 'https://play.google.com/store/apps/details?id=' + app
                else:
                    fb = url
                expected_uri = 'intent://#Intent;package=' + app + ';S.browser_fallback_url=' + urllib2.quote(fb, '') + ';end'
            else:
                expected_uri = url
            assert uri == expected_uri, uri + ' != ' + expected_uri
            has_open_suggest = True
        if not has_serp and block['type'] == 'suggest' and block['suggest_type'] == 'open_serp_fallback':
            has_serp = True
            uri = block['data']['url']
            q = urllib2.quote(query.encode('utf-8'))
            if os == 'windows':
                expected_uri = 'https://yandex.ru/search/?text=' + q
            else:
                expected_uri = 'viewport://?noreask=1&text=' + q + '&viewport_id=serp'
            assert uri == expected_uri, uri + ' != ' + expected_uri

    assert has_open_suggest
    assert has_serp
    print 'ok'


def test(query, gplay, itunes, mobile_url, desktop_url, title):
    # if mobile_url.find('yandex.ru') != -1 or desktop_url.find('yandex.ru') != -1:
    #     print 'skip'
    #     return
    if gplay != '':
        if mobile_url == '' and desktop_url == '':
            url = 'https://play.google.com/store/apps/details?id=' + gplay
        elif mobile_url != '':
            url = mobile_url
        else:
            url = desktop_url
        test0(query, 'app', 'ru.yandex.searchplugin/10.0', 'android', title, gplay, url)
        test0(query, 'site', 'ru.yandex.searchplugin/10.0', 'android', title, '', url)
        test0(query, '', 'ru.yandex.searchplugin/10.0', 'android', title, gplay, url)
    if itunes != '':
        test0(query, 'app', 'ru.yandex.mobile/10.0', 'ios', title, itunes, mobile_url)
        test0(query, 'site', 'ru.yandex.mobile/10.0', 'ios', title, '', mobile_url)
        test0(query, '', 'ru.yandex.mobile/10.0', 'ios', title, itunes, mobile_url)
    if gplay == '' and itunes == '':
        test0(query, 'app', 'ru.yandex.searchplugin/10.0', 'android', title, '', mobile_url)
        test0(query, 'site', 'ru.yandex.searchplugin/10.0', 'android', title, '', mobile_url)
        test0(query, '', 'ru.yandex.searchplugin/10.0', 'android', title, '', mobile_url)
        test0(query, 'app', 'ru.yandex.mobile/10.0', 'ios', title, '', mobile_url)
        test0(query, 'site', 'ru.yandex.mobile/10.0', 'ios', title, '', mobile_url)
        test0(query, '', 'ru.yandex.mobile/10.0', 'ios', title, '', mobile_url)
    if desktop_url != '':
        test0(query, '', 'winsearchbar/10.0', 'windows', title, '', desktop_url)


with codecs.open(fixlist_in, 'r', encoding='utf-8') as fin:
    i = 0
    for line in fin:
        if i <= offset:
            i = i + 1
            continue
        parts = line.strip('\r\n').split('\t')
        query = parts[0].strip()
        print u'{}: \'{}\''.format(i, query)
        test(query, parts[1].strip(), parts[2].strip(), parts[3].strip(), parts[4].strip(), parts[5].strip())
        i = i + 1
