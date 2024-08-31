# encoding: utf-8

from .test_utils import (
    calc_specific_metric
)


def test_nlg_metrics_01():
    result = calc_specific_metric(['good', 'good'], ['good', 'good'],
                                  'nlg_empty', basket='ue2e_quasar', metrics_groups=['nlg'],
                                  prod_answers=['ок', ''], test_answers=[None, ''])

    assert result == {
        'basket': 'ue2e_quasar',
        'metric_name': 'nlg_empty',
        'metric_description': "Доля запросов с пустым ответом Алисы (ответ None или пустая строка)\n    Как в АБ метрика 'Доля пустых запросов'",
        'metrics_group': 'nlg',
        'prod_quality': 0.5,
        'test_quality': 1.0,
        'diff': 0.5,
        'diff_percent': 100.0,
        'pvalue': 0.49999999999999956,
    }


def test_nlg_metrics_02():
    result = calc_specific_metric(['good', 'good'], ['good', 'good'],
                                  'nlg_all_not_found', basket='ue2e_quasar', metrics_groups=['nlg'],
                                  prod_answers=['К сожалению, ничего не нашлось.', 'Я не поняла, что повторить?'],
                                  test_answers=['ок', 'Извините, у меня нет хорошего ответа.'])

    assert result == {
        'basket': 'ue2e_quasar',
        'metric_name': 'nlg_all_not_found',
        'metric_description': ('Доля запросов с ответами-заглушками типа \'я ничего не нашла, извините\'\n    ' +
                               'Ответы из интентов hancrafted и reask а также включает в себя ' +
                               'топ популярных ответов из других сценариев'),
        'metrics_group': 'nlg',
        'prod_quality': 1.0,
        'test_quality': 0.5,
        'diff': -0.5,
        'diff_percent': -50.0,
        'pvalue': 0.49999999999999956,
    }


def test_nlg_metrics_03():
    result = calc_specific_metric(['good', 'good'], ['good', 'good'],
                                  'nlg_errors', basket='ue2e_quasar', metrics_groups=['nlg'],
                                  prod_answers=['Прошу прощения, что-то сломалось.', 'Мне кажется, меня уронили.'],
                                  test_answers=['ок', 'Произошла какая-то ошибка.'])

    assert result == {
        'basket': 'ue2e_quasar',
        'metric_name': 'nlg_errors',
        'metric_description': ('Доля запросов с ответами Алисы типа \'Прошу прощения, что-то сломалось.\'\n    ' +
                               'Список ответов из common_nlg phrase error\n    ' +
                               'Как в АБ метрика \'Доля "Что-то пошло не так"\''),
        'metrics_group': 'nlg',
        'prod_quality': 1.0,
        'test_quality': 0.5,
        'diff': -0.5,
        'diff_percent': -50.0,
        'pvalue': 0.49999999999999956,
    }


def test_nlg_metrics_04():
    result = calc_specific_metric(['good', 'good', 'good'], ['good', 'good', 'good'],
                                  'nlg_sorry', basket='ue2e_quasar', metrics_groups=['nlg'],
                                  prod_answers=['Извините блаблабла', 'прошу прощения', 'ок'],
                                  test_answers=['я БОЮСЬ ничем не могу помочь', 'ок', 'ок'])

    assert result == {
        'basket': 'ue2e_quasar',
        'metric_name': 'nlg_sorry',
        'metric_description': ('Доля запросов с ответами Алисы содержащими подстроки типа \'извините\', \'простите\' ' +
                               'и т.п.\n    Как в АБ метрика \'Доля "Извините"\''),
        'metrics_group': 'nlg',
        'prod_quality': 0.6666666666666666,
        'test_quality': 0.3333333333333333,
        'diff': -0.3333333333333333,
        'diff_percent': -49.99999999999999,
        'pvalue': 0.42264973081037427,
    }
