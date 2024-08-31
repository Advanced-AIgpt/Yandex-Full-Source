from alice.analytics.operations.priemka.metrics_calculator.metric import Metric

from math import log2

MIN_PENALTY_FOR_REL_WITH_MAIN_TARGET = 0.05
MIN_PENALTY_FOR_REL_MINUS_WITH_MAIN_TARGET = 0.07

PENALTIES = {
    'target_before_target': 0,
    'target_after_target': 0,
    'rel_after_target': 0,
    'rel_minus_after_target': 0,
    'rel_before_target': 0.5,
    'rel_minus_before_target': 0.75,
    'stupid_after_target': 0.8,
    'stupid_before_target': 1,
}


def dcg(rel_list):
    return sum([(2 ** PENALTIES[rel] - 1) / log2(pos + 2) for pos, rel in rel_list])


class CallsQuality(Metric):

    #  https://wiki.yandex-team.ru/users/olesyal7/metrika-zvonkov/

    def group(self):
        return 'calls'

    def get_answer(self, item):
        return [x['relevance'] for x in item['answer']]

    def calc_metric(self, item, per_page, pages_num):
        answer = self.get_answer(item)
        if 'target' not in answer:
            return 0
        target_idx = answer.index('target')
        max_ = 1 - (target_idx == 0) * max(MIN_PENALTY_FOR_REL_WITH_MAIN_TARGET * ('rel' in answer),
                                           MIN_PENALTY_FOR_REL_MINUS_WITH_MAIN_TARGET * ('rel_minus' in answer)) * ('stupid' not in answer)
        min_ = max(0.1, 0.5 * (target_idx < per_page) * (pages_num > 1), 0.2 * (target_idx < 2 * per_page) * (pages_num > 2))
        norm = dcg(enumerate(['stupid_before_target'] * pages_num * per_page))
        rel_list = [(pos, rel + (pos <= target_idx) * '_before_target' + (pos > target_idx) * '_after_target')
                    for pos, rel in enumerate(answer)]
        return round(max(max_ - dcg(rel_list) / norm, min_), 2)


class CallsQualitySearchApp(CallsQuality):

    #  https://wiki.yandex-team.ru/users/olesyal7/metrika-zvonkov/

    def value(self, item):
        return self.calc_metric(item, 5, 1)


class CallsQualityCloud(CallsQuality):

    #  https://wiki.yandex-team.ru/users/olesyal7/metrika-zvonkov/

    def value(self, item):
        return self.calc_metric(item, 3, 2)


class MessengerCallsScenario(Metric):

    #  https://wiki.yandex-team.ru/users/olesyal7/metrika-zvonkov/

    def group(self):
        return 'calls'

    def value(self, item):
        return int(item["intent"] == "phone_call")


class OnlyTargetAnswer(CallsQuality):

    #  https://wiki.yandex-team.ru/users/olesyal7/metrika-zvonkov/

    def group(self):
        return 'calls'

    def value(self, item):
        return int(self.get_answer(item) == ['target'])


class WithTargetAnswer(CallsQuality):

    #  https://wiki.yandex-team.ru/users/olesyal7/metrika-zvonkov/

    def group(self):
        return 'calls'

    def value(self, item):
        return int('target' in self.get_answer(item))


class FirstTargetAnswer(CallsQuality):

    #  https://wiki.yandex-team.ru/users/olesyal7/metrika-zvonkov/

    def group(self):
        return 'calls'

    def value(self, item):
        answer = self.get_answer(item)
        return int((answer != []) and (answer[0] == 'target'))
