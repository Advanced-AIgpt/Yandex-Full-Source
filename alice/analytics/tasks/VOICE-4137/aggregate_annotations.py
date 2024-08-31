# -*- coding=utf8 -*-
import argparse
import json
import logging
import random
import sys
from collections import defaultdict


class Answer:
    """
        здесь собираем информацио об одном (нормализованном) ответе
        (список - кто из толокёров его набрал + наиболее удобная для WER ненормализованная версия (TR))
    """
    def __init__(self):
        self.text = None
        self.workers = []

    def append_worker(self, worker_id, text):
        self.workers.append(worker_id)
        self.update_text(text)

    def update_text(self, text):
        if not self.text:
            self.text = text
        elif len(text) > len(self.text):
            # for turkish choose text with maximum (space) chars
            # для турецкого из ненормализованных текстов выбираем самый большой
            # (с наибольшим количеством пробелов) для наиболее корректного расчёта WER-a
            self.text = text

    def count_workers(self):
        return len(self.workers)

    def __repr__(self):
        return 'Answer(text={}, worker_cnt={})'.format(self.text.encode('utf-8'), len(self.workers))


def setup_logger():
    root_logger = logging.getLogger('')
    file_logger = logging.StreamHandler()
    file_logger.setFormatter(logging.Formatter('%(asctime)s %(levelname)-8s %(filename)s:%(lineno)d %(message)s'))
    root_logger.addHandler(file_logger)
    root_logger.setLevel(logging.DEBUG)


def parse_cmd_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input', dest='input', help='json with assignments - sb urls')
    parser.add_argument('--lang', metavar='LANG', help='input language (ru|tr)')
    parser.add_argument('-g', '--good', dest='good', help='good results')
    parser.add_argument('-b', '--bad', dest='bad', help='bad results')
    parser.add_argument('-c', '--confusing', dest='confusing', help='tasks for recheck')
    parser.add_argument('-d', '--data', dest='data', help='answers for tasks to recheck')
    parser.add_argument('-hp', '--honeypots', dest='honeypots', help='honeypots created')
    parser.add_argument('-ch', '--check', dest='check', help='json to check the assignments')
    parser.add_argument('--istest', help='Should the whole sample be used for test? default: False',
                        action="store_true")
    parser.add_argument('--speech_field', help='Should the speech field be presented in result? default: False',
                        action="store_true")

    context = parser.parse_args(sys.argv[1:])

    if not context.lang:
        raise Exception('need LANG')

    return context


def write_result(name, result, context, is_json_list=False):
    logging.debug("{} results: {}".format(name, len(result)))
    with open(getattr(context, name), 'w') as out:
        if is_json_list:
            for obj in result:
                json.dump(obj, out)
                out.write('\n')
        else:
            json.dump(result, out, indent=4, encoding='utf-8')


def process(context):
    assignments = json.loads(open(context.input).read())
    logging.debug("load {} assignments".format(len(assignments)))

    processor = Processor(assignments, context)
    logging.debug("processing {} answers ...".format(len(processor.answers_text)))
    processor.run()

    excluded_assigns = [check for check in processor.check_results
                        if check['status']['value'] == 'REJECT_SUBMITTED']

    excluded_assign_ids = {check['assignmentId'] for check in excluded_assigns}

    assignments = [a for a in assignments
                   if a['assignmentId'] not in excluded_assign_ids]

    processor = Processor(assignments, context)
    logging.debug("after filtering {} bad assignments, processing {} answers ...".format(
        len(excluded_assigns), len(processor.answers_text))
    )
    processor.run()

    write_result('good', processor.good, context, is_json_list=True)
    write_result('bad', processor.bad, context)
    write_result('data', processor.confusing_answers, context)
    write_result('confusing', processor.confusing_tasks, context)
    write_result('honeypots', processor.honeypots, context)
    write_result('check', processor.check_results + excluded_assigns, context)


class Processor(object):
    def __init__(self, assignments, context):
        self.assignments = assignments
        self.prepare_data(context.lang)

        self.good = []
        self.bad = []
        self.confusing_tasks = []
        self.confusing_answers = {}
        self.honeypots = []

        # Оценка правильности выполненных заданий
        self.completion = defaultdict(lambda: {'correct': 0.0, 'total': 0.0})  # ключи - assignmentId
        self.check_results = []

        self.speech_field = context.speech_field
        if context.istest:
            self.make_mark = lambda: 'TEST'
        else:
            self.make_mark = self.make_random_mark

    def prepare_data(self, lang):
        self.answers_text = defaultdict(lambda: defaultdict(Answer))
        self.answers_speech = defaultdict(lambda: defaultdict(Answer))
        self.public_urls = {}  # {url: {'publicUrl': public_url, 'assignmentId': assignment_id}
        self.evaluation = defaultdict(lambda: defaultdict(list))  # {url: {text: [assignment_ids]}}
        for item in self.assignments:
            result = item['outputValues']['result']
            speech = item['outputValues']['speech']
            text_processed = None
            if speech != 'BAD' and result:
                if lang == 'ru':
                    # нормализуем ё->е
                    text = result.lower().replace(u'ё', u'е')
                elif lang == 'tr':
                    text = result.lower()
                    # в турецком пробелы могут пропускаться, так что нормализуем строку в безпробельную
                    text_processed = u"".join(text.split())
                else:
                    raise Exception('unknown lang')
            else:
                text = 'BAD'
            if text_processed is None:
                text_processed = text
            self.answers_text[item['inputValues']['url']][text_processed].append_worker(item['workerId'], text)
            if speech != 'BAD':
                self.answers_speech[item['inputValues']['url']][speech].append_worker(item['workerId'], speech)
            self.public_urls[item['inputValues']['url']] = {'publicUrl': item['publicUrl'], 'assignmentId': item['assignmentId']}
            self.evaluation[item['inputValues']['url']][text_processed].append(item['assignmentId'])

    def run(self):
        for url in self.answers_text:
            self.process_url(url)
        self.check_assignments()

    @staticmethod
    def make_random_mark():
        rnd = random.random()
        if rnd < 0.6:
            return "TRAIN"
        elif rnd > 0.8:
            return "DEV"
        else:
            return "TEST"

    @staticmethod
    def sum_answer_workers((text, answer)):
        return (answer.count_workers(), text != 'BAD')  # При одинаковой авторитетности, нужно выбирать не 'BAD'

    def process_url(self, url):
        url_texts = self.answers_text[url]
        url_speech = self.answers_speech[url]
        # получаем (text_processed, Answer), с максимальным кол-вом толокёров на одну общую
        # (нормализованную) версию аннотации, т.е. самую авторитетную версию
        most_rated_norm_text, most_rated_answers = max(url_texts.iteritems(), key=self.sum_answer_workers)
        # ответы группируются по нормализованному тексту
        # для расчёта WER используем ненормализованный текст - именно его пишем в результаты
        most_rated_text = most_rated_answers.text

        rating_speech = sorted(url_speech.iteritems(), key=self.sum_answer_workers)

        best_rate = most_rated_answers.count_workers()

        most_rated_speech = "BAD"
        if rating_speech:
            most_rated_speech = rating_speech[-1][1].text
            best_rate_speech = rating_speech[-1][1].count_workers()
            if len(rating_speech) > 1 and best_rate_speech == rating_speech[-2][1].count_workers():
                most_rated_speech = "UNDEF"

        if most_rated_text != "BAD" and best_rate > 1:
            record = {"text": most_rated_text, "url": url, "voices": best_rate, "mark": self.make_mark()}
            if self.speech_field:
                record["speech"] = most_rated_speech
            self.good.append(record)

            if (most_rated_text and
                    len(url_texts) == 1 and  # Нет расхождений по тексту
                    url_texts[most_rated_norm_text].count_workers() >= 3):
                if len(url_speech) == 1:  # Нет расхождений по оценке речи
                    self.honeypots.append({"inputValues": {"url": url},
                                           "outputValues": {"result": most_rated_text, "speech": most_rated_speech}})
                elif not self.speech_field:  # Речь не оценивалась
                    self.honeypots.append({"inputValues": {"url": url},
                                           "outputValues": {"result": most_rated_text}})

            self.update_completions(url, most_rated_norm_text)

        elif most_rated_text == "BAD" and best_rate > 2:
            self.bad.append({"text": most_rated_text, "url": url, "voices": best_rate})

            self.update_completions(url, most_rated_norm_text)

        else:
            self.confusing_tasks.append({"inputValues": {"url": self.public_urls[url]['publicUrl']}})
            # data = dict(text, [worker_id, ...])
            data = {}
            for norm_text, answer in self.answers_text[url].iteritems():
                data[answer.text] = answer.workers
            self.confusing_answers[url] = {'data': data}

    def update_completions(self, url, most_rated_norm_text):
        for text, assignment_ids in self.evaluation[url].iteritems():
            for assign_id in assignment_ids:
                counters = self.completion[assign_id]
                counters['total'] += 1
                if text == most_rated_norm_text:
                    counters['correct'] += 1

    def check_assignments(self):
        pool_id = self.assignments[0]['poolId']
        for assign_id, counters in self.completion.iteritems():
            try:
                if (counters['correct'] / counters['total']) >= 0.25:
                    status = {"comment": "Thank you", "value": "APPROVE_SUBMITTED"}
                else:
                    status = {"comment": "Too many incorrect tasks", "value": "REJECT_SUBMITTED"}
                self.check_results.append({'assignmentId': assign_id, 'status': status, 'poolId': pool_id})
            except Exception as e:
                logging.exception('assignment %s problem: %s', assign_id, e)


def main():
    setup_logger()
    context = parse_cmd_args()
    process(context)


if __name__ == '__main__':
    main()
