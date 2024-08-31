# encoding: utf-8

from .test_utils import calc_specific_metric


def test_asr_metrics_01_wer():
    prod_texts = ['слушать музыку новинки', 'карусель', 'одна минута назад']
    prod_asr_texts = ['слушать музыку налим', '', 'одну минуту назад']
    test_texts = ['послезавтра', 'включи с начала', 'сказки']
    test_asr_texts = ['послезавтра', 'погода', 'включи сказки']
    result = calc_specific_metric(
        prod_texts, test_texts, 'wer', basket='ue2e_quasar', metrics_groups=['asr'],
        asr_prod_texts=prod_asr_texts, asr_test_texts=test_asr_texts)

    assert result == {
        'metric_name': 'wer',
        'metric_description': 'WER - word error rate — метрика качества ASR\n    Считается по паре (text, asr_text) на запросах без неответов\n    ',
        'metrics_group': 'asr',
        'pvalue': 0.9999999999999998,
        'basket': 'ue2e_quasar',
        'prod_quality': 57.14285714285714,
        'test_quality': 80.0,
        'diff': 22.85714285714286,
        'diff_percent': 40.000000000000014
    }


def test_asr_metrics_02_wer():
    prod_texts = ['слушать музыку новинки', 'карусель', 'одна минута назад']
    prod_asr_texts = ['слушать музыку налим', '', 'одну минуту назад']
    result = calc_specific_metric(
        prod_texts, [], 'wer', basket='ue2e_quasar', metrics_groups=['asr'], asr_prod_texts=prod_asr_texts)

    assert result == {
        'metric_name': 'wer',
        'metric_description': 'WER - word error rate — метрика качества ASR\n    Считается по паре (text, asr_text) на запросах без неответов\n    ',
        'metrics_group': 'asr',
        'basket': 'ue2e_quasar',
        'metric_value': 57.14285714285714,
        'metric_num': 171.42857142857142,
        'metric_denom': 3
    }


def test_asr_metrics_03_werp():
    prod_texts = ['слушать музыку новинки', 'карусель', 'одна минута назад']
    prod_asr_texts = ['слушать музыку налим', '', 'одну минуту назад']
    test_texts = ['послезавтра', 'включи с начала', 'сказки']
    test_asr_texts = ['послезавтра', 'погода', 'включи сказки']
    result = calc_specific_metric(
        prod_texts, test_texts, 'werp', basket='ue2e_quasar', metrics_groups=['asr'],
        asr_prod_texts=prod_asr_texts, asr_test_texts=test_asr_texts)

    assert result == {
        'metric_name': 'werp',
        'metric_description': 'WERP - Улучшенная метрика качества ASR — WER + Phonemes, учитывающая фонемы\n    Считается по паре (text, asr_text) на запросах без неответов',
        'metrics_group': 'asr',
        'pvalue': 0.9028904978761064,
        'basket': 'ue2e_quasar',
        'prod_quality': 67.22222222222221,
        'test_quality': 60.55555555555555,
        'diff': -6.666666666666664,
        'diff_percent': -9.917355371900824
    }


def test_asr_metrics_04_empty_answers():
    prod_texts = ['', '', '', '']
    prod_asr_texts = ['текст один', 'текст два', '', '']
    test_texts = ['текст один', 'текст два', 'текст три', 'текст четыре']
    test_asr_texts = ['', '', '', 'текст чепыре']

    assert calc_specific_metric(
        prod_texts, test_texts, metric_name='empty_answers_false_supression_rate',
        basket='ue2e_quasar', metrics_groups=['asr'], asr_prod_texts=prod_asr_texts, asr_test_texts=test_asr_texts
    ) == {
        'metric_name': 'empty_answers_false_supression_rate',
        'metrics_group': 'asr',
        'prod_quality': 0.0,
        'test_quality': 0.75,
        'diff': 0.75,
        'diff_percent': 0,
        'basket': 'ue2e_quasar'
    }

    assert calc_specific_metric(
        prod_texts, test_texts, metric_name='empty_answers_precision',
        basket='ue2e_quasar', metrics_groups=['asr'], asr_prod_texts=prod_asr_texts, asr_test_texts=test_asr_texts
    ) == {
        'metric_name': 'empty_answers_precision',
        'metrics_group': 'asr',
        'prod_quality': 1.0,
        'test_quality': 0.0,
        'diff': -1.0,
        'diff_percent': -100.0,
        'basket': 'ue2e_quasar'
    }

    assert calc_specific_metric(
        prod_texts, test_texts, metric_name='empty_answers_recall',
        basket='ue2e_quasar', metrics_groups=['asr'], asr_prod_texts=prod_asr_texts, asr_test_texts=test_asr_texts
    ) == {
        'metric_name': 'empty_answers_recall',
        'metrics_group': 'asr',
        'prod_quality': 0.5,
        'test_quality': 0.0,
        'diff': -0.5,
        'diff_percent': -100.0,
        'basket': 'ue2e_quasar'
    }

    assert calc_specific_metric(
        prod_texts, test_texts, metric_name='empty_answers_supression_rate',
        basket='ue2e_quasar', metrics_groups=['asr'], asr_prod_texts=prod_asr_texts, asr_test_texts=test_asr_texts
    ) == {
        'metric_name': 'empty_answers_supression_rate',
        'metrics_group': 'asr',
        'prod_quality': 0.5,
        'test_quality': 0.75,
        'diff': 0.25,
        'diff_percent': 50.0,
        'basket': 'ue2e_quasar'
    }
