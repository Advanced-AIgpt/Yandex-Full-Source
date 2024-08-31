#!/usr/bin/env python
# encoding: utf-8

import requests
from multiprocessing.dummy import Pool as ThreadPool
import logging
import random
import time

from utils.nirvana.op_caller import call_as_operation

URL = "http://vins.hamster.alice.yandex.net/qa/pa/nlu/"

APP_LIST = {
    "search_app_prod": "ru.yandex.searchplugin",
    "search_app_beta": "ru.yandex.searchplugin.beta",
    "stroka": "winsearchbar",
    "stroka_browser": "winsearchbar",
    "browser_prod": "com.yandex.browser",
    "browser_alpha": "com.yandex.browser.alpha",
    "browser_beta": "com.yandex.browser.beta"
}

SLEEP_EPSILON = 1

DEFAULT_UUID = "00226124-3902-44d8-88bd-3318700cec29"

HEADERS = {'Accept-Encoding': ''}


def random_hash():
    return ''.join(map(lambda x: random.choice("0123456789abcdef"), range(32)))


def get_payload(app, reqid, text, type_intent, toloka_intent, vins_prev_intent=""):
    return {
        "header": {
            "request_id": random_hash()
        },
        "application": {
            "app_id": APP_LIST.get(app, 'ru.yandex.searchplugin'),
            "app_version": "72.3",
            "os_version": "5.0",
            "platform": "android",
            "uuid": reqid,
            "lang": "ru-RU",
            "timezone": "Europe/Moscow",
            "timestamp": "1504030201"
        },
        "request": {
            type_intent: vins_prev_intent,
            "true_intent": toloka_intent,
            "event": {
                "type": "text_input",
                "text": text
            }
        }
    }


def check_prev_intent(app, reqid, text, toloka_intent, vins_prev_intent=""):
    data = get_payload(app, reqid, text, "pred_intent", toloka_intent, vins_prev_intent)
    num_retry = 0
    try:
        r = requests.post(URL, json=data, verify=False, headers=HEADERS)
    except Exception as e:
        logging.error('%s - %s performed %d retries',
                      e, 're-classification', num_retry)
        return False
    while r.status_code != 200 and num_retry < 3:
        time.sleep(SLEEP_EPSILON)
        try:
            r = requests.post(URL, json=data, verify=False, headers=HEADERS)
            num_retry += 1
            pass
        except Exception as e:
            num_retry += 1
            logging.error('%s %s - %s performed %d retries',
                          e, r.status_code, 're-classification', num_retry)
    try:
        ans = r.json()
        if ans['true_intent'] == ans['pred_intent']:
            return ans['true_intent']
        else:
            return False
    except Exception as e:
        logging.error('%s %s - performed %d retries',
                      e, r.status_code, num_retry)
        return False


def predict_intent(app, reqid, text, toloka_intent, vins_prev_intent=""):
    data = get_payload(app, reqid, text, "prev_intent", toloka_intent, vins_prev_intent)
    num_retry = 0
    try:
        r = requests.post(URL, json=data, verify=False, headers=HEADERS)
    except Exception as e:
        logging.error('%s - %s performed %d retries',
                      e, text, num_retry)
        return {'text': text, 'intent': 'ERROR', 'toloka_intent': 'ERROR'}
    while r.status_code != 200 and num_retry < 3:
        time.sleep(SLEEP_EPSILON)
        try:
            r = requests.post(URL, json=data, verify=False, headers=HEADERS)
            num_retry += 1
            pass
        except Exception as e:
            num_retry += 1
            logging.error('%s %s - %s performed %d retries',
                          e, r.status_code, text, num_retry)
    try:
        intent = r.json()['semantic_frames'][0]['intent_name']
        toloka_intent = r.json()['true_intent']
        if num_retry > 0:
            logging.warning('%s - %s performed %d retries',
                            r.status_code, text, num_retry)
        return {'text': text, 'intent': str(intent), 'toloka_intent': str(toloka_intent)}
    except Exception as e:
        logging.error('%s %s - %s performed %d retries',
                      e, r.status_code, text, num_retry)
        return {'text': text, 'intent': 'ERROR', 'toloka_intent': 'ERROR'}


def process_item(item):
    if item.get('reqid', DEFAULT_UUID) == u"None":
        return None
    classification = predict_intent(item['app'], item.get('reqid', DEFAULT_UUID), item['dialog'][-1],
                                    item.get('toloka_intent', ''), item.get('vins_prev_intent', ''))
    if len(item['dialog']) > 1:
        if check_prev_intent(item['app'], item.get('reqid', DEFAULT_UUID), item['dialog'][-1],
                             item['prev_toloka_intent'], item.get('vins_prev_intent', '')):
            return classification
        else:
            return None
    return classification


def get_prediction(bucket):
    pool = ThreadPool(8)
    results = pool.map(process_item, bucket)
    pool.close()
    pool.join()
    return results


def main(vins_url, bucket):
    global URL
    URL = vins_url

    results = get_prediction(bucket)
    return [x for x in results if x is not None]


if __name__ == '__main__':
    call_as_operation(main, input_spec={'bucket': {'required': True, 'parser': 'json'}})
