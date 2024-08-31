import json
import requests
import argparse
import sys
import collections
import threading
import time

SLEEP_MUL = 2

urlClassifier = 'http://vins-int.dev.voicetech.yandex.net'
handler = '/qa/nlu/pa/'
app_id = 'ru.yandex.searchplugin'


def get_intent(utterance, retries=5, sleep_s=1):
    for i in range(retries):
        try:
            params = {'utterance': utterance,
                      'app_id': app_id}
            r = requests.get(urlClassifier + handler, params=params, verify=False)
            print >>sys.stderr, r.status_code
            answer = json.loads(r.text)
            if not answer:
                continue
                # return None

            intent = answer['semantic_frames'][0]['intent_name']
            return intent
        except:
            time.sleep(sleep_s)
            sleep_s *= SLEEP_MUL
    return None


def Download(threadNumber, threadsCount, ids, counters):
    f = open("sample_%s" % threadNumber, 'w')
    for i in range(len(ids)):
        if i % threadsCount != threadNumber:
            continue
        try:
            s = ids[i]['request'].strip()
            intent = get_intent(s)
            # print >>f, s, "|", intent
            print >>f, json.dumps({'request': ids[i]['request'], 'intent': intent, 'id': ids[i]['id']})
            counters[threadNumber][intent] += 1
        except Exception as ex:
            print >>sys.stderr, ex

    f.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--output-dict', required=True)
    parser.add_argument('--url', required=True)
    parser.add_argument('--app', default='ru.yandex.searchplugin')

    args = parser.parse_args()

    global urlClassifier
    urlClassifier = args.url

    global app_id
    app_id = args.app

    threadsCount = 10
    threads = []

    ids = []
    with open(args.input, 'r') as f:
        ids = json.loads(f.read())

    counters = []
    for i in range(threadsCount):
        counters.append(collections.Counter())

    for threadNumber in range(threadsCount):
        t = threading.Thread(target=Download, args=[threadNumber, threadsCount, ids, counters])
        t.daemon = True
        t.start()
        threads.append(t)

    for t in threads:
        t.join()

    c = collections.Counter()
    results = []
    for i in range(threadsCount):
        c += counters[i]
        try:
            with open("sample_%s" % i) as f:
                for j in f:
                    results.append(json.loads(j))
        except Exception as ex:
            print >>sys.stderr, ex

    with open(args.output, 'w') as f:
        f.write(json.dumps(results, indent=4, ensure_ascii=False).encode('utf-8'))

    with open(args.output_dict, 'w') as f:
        s = json.dumps(c, ensure_ascii=False).encode('utf-8')
        f.write(s)

if __name__ == '__main__':
    main()
