# coding: utf-8

import voicetech.asr.tools.metrics.lib.metric as metric
from voicetech.asr.tools.metrics.lib.record import Sample, Record, Response
from alice.analytics.operations.priemka.metrics_calculator.metric import Metric


def sample_ref_hypo(reference, hypo):
    return Sample(
        record=Record(reference=reference),
        response=Response(hypos=[hypo]),
    )


class AsrMetric(Metric):
    def group(self):
        return 'asr'

    def aggregate_type(self):
        return None


class AsrAggregatedMetric(AsrMetric):
    def aggregate_type(self):
        return 'custom'

    def statistical_test(self):
        return None

    def compare_asr(self, prod, test, metric_computer, metric_name=None, allow_sidespeech=False):
        computer_prod = metric_computer()
        for (text, asr_text) in prod:
            if asr_text is None or (text == '' and not allow_sidespeech):
                # в случае неответа или сайдспича не считаем ASR метрики
                continue
            computer_prod.update(sample_ref_hypo(text, asr_text))
        result_prod = computer_prod.compute().get(metric_name) if metric_name else computer_prod.compute()

        if test is None:
            # расчёт для одной системы: metric_num, metric_denom, metric_value
            return len(prod) * result_prod, len(prod), result_prod

        computer_test = metric_computer()
        for (text, asr_text) in test:
            if asr_text is None or (text == '' and not allow_sidespeech):
                # в случае неответа или сайдспича не считаем ASR метрики
                continue
            computer_test.update(sample_ref_hypo(text, asr_text))
        result_test = computer_test.compute().get(metric_name) if metric_name else computer_test.compute()

        diff = result_test - result_prod
        diff_percent = (100.0 * diff / result_prod) if result_prod != 0 else 0
        return result_prod, result_test, diff, diff_percent


class WerAggregated(AsrAggregatedMetric):
    """
    Класс метрики WER для расчёта абсолютных значений метрики и диффа между prod и test
    Используется в alice metrics calculator, не считает стат.значимость
    """
    def value(self, item):
        if item.get('voice_url') is None:
            return None, None
        return item.get('text'), item.get('asr_text')

    def compare(self, prod, test):
        return self.compare_asr(prod, test, metric.WerMetric)


# Класс метрики WER для расчёта:
#     * посемплового значения метрики для yt metrics calculator
#     * для расчёта стат.значимости в массивах посемпловых значений в alice metrics calculator
# Не умеет считать метрику на всей корзинке и дифф (т.к. в метрике своя аггрегация)
class Wer(AsrMetric):
    """WER - word error rate — метрика качества ASR
    Считается по паре (text, asr_text) на запросах без неответов
    """
    def value(self, item):
        if item.get('asr_text') is None or item.get('text') == '' or item.get('voice_url') is None:
            # в случае неответа или сайдспича не считаем ASR метрики, или запрос без голоса (текстом)
            return None
        computer = metric.WerMetric()
        sample = sample_ref_hypo(item.get('text'), item.get('asr_text'))
        return computer.compute_single(sample)


class WerpAggregated(AsrAggregatedMetric):
    """
    Класс метрики WERP для расчёта абсолютных значений метрики и диффа между prod и test
    Используется в alice metrics calculator, не считает стат.значимость
    """
    def value(self, item):
        if item.get('voice_url') is None:
            return None, None
        return item.get('text'), item.get('asr_text')

    def compare(self, prod, test):
        return self.compare_asr(prod, test, metric.WerpMetric)


# Класс метрики WERP для расчёта:
#     * посемплового значения метрики для yt metrics calculator
#     * для расчёта стат.значимости в массивах посемпловых значений в alice metrics calculator
# Не умеет считать метрику на всей корзинке и дифф (т.к. в метрике своя аггрегация)
class Werp(AsrMetric):
    """WERP - Улучшенная метрика качества ASR — WER + Phonemes, учитывающая фонемы
    Считается по паре (text, asr_text) на запросах без неответов"""
    def value(self, item):
        if item.get('asr_text') is None or item.get('text') == '' or item.get('voice_url') is None:
            # в случае неответа или сайдспича не считаем ASR метрики, или запрос без голоса (текстом)
            return None
        computer = metric.WerpMetric()
        sample = sample_ref_hypo(item.get('text'), item.get('asr_text'))
        return computer.compute_single(sample)


class WordError(AsrAggregatedMetric):
    """Число ошибок ASR, числитель WER
    Считается по паре (text, asr_text) на запросах без неответов"""
    def value(self, item):
        if item.get('voice_url') is None:
            return None, None
        return item.get('text'), item.get('asr_text')

    def compare(self, prod, test):
        return self.compare_asr(prod, test, metric.WordErrorMetric)


class EmptyAnswersPrecision(AsrAggregatedMetric):
    def value(self, item):
        if item.get('voice_url') is None:
            return None, None
        return item.get('text'), item.get('asr_text')

    def compare(self, prod, test):
        return self.compare_asr(prod, test, metric.EmptyAnswersMetric, 'precision', allow_sidespeech=True)


class EmptyAnswersRecall(AsrAggregatedMetric):
    def value(self, item):
        if item.get('voice_url') is None:
            return None, None
        return item.get('text'), item.get('asr_text')

    def compare(self, prod, test):
        return self.compare_asr(prod, test, metric.EmptyAnswersMetric, 'recall', allow_sidespeech=True)


class EmptyAnswersSupressionRate(AsrAggregatedMetric):
    def value(self, item):
        if item.get('voice_url') is None:
            return None, None
        return item.get('text'), item.get('asr_text')

    def compare(self, prod, test):
        return self.compare_asr(prod, test, metric.EmptyAnswersMetric, 'supression_rate', allow_sidespeech=True)


class EmptyAnswersFalseSupressionRate(AsrAggregatedMetric):
    def value(self, item):
        if item.get('voice_url') is None:
            return None, None
        return item.get('text'), item.get('asr_text')

    def compare(self, prod, test):
        return self.compare_asr(prod, test, metric.EmptyAnswersMetric, 'false_supression_rate', allow_sidespeech=True)
