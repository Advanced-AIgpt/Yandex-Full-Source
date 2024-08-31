# coding: utf-8

import os
import json
from copy import deepcopy
from string import Template

ERROR_THRESHHOLDS = {
    'vins_sources_error': 0.075,
    'preparer_error': 0.0075,
    'uniproxy_error': 0.015,

    'render_error': 0.03,  # ошибки рендера скриншотов скриншотилкой
    'toloka_bad_url_error': 0.015,  # битые скриншоты, отмеченные толокерами

    # все ошибки прокачки вместе (эквивалентны uniproxy_error)
    'all_download_error': 0.015,
    'mm_unanswer_error': 0.015,
    'downloader_error': 0.015,
    'tunneler_error': 0.015,
    'unknown_error': 0.015,
    'pser_binary': 0.05,  # при доле ошибок PSER > 5%
}
OTHER_ERROR_THRESHHOLD = 0.015

ERROR_THRESHHOLDS_BY_BASKET = {
    'e2e_quasar_item_selector': {
        'all_download_error': 0.05,
        'mm_unanswer_error': 0.05,
        'uniproxy_error': 0.05,
    },
    'e2e_smart_home_from_online': {
        # 8 битых скриншотов из среза 128 запросов ПП недопустимы
        'toloka_bad_url_error': 0.0625,
    },
}

# VA-2316 минимально допустимое значение метрики в продовой ветке по корзинкам (μ - 3σ)
BASKETS_MIN_PROD_QUALITY_VALUE = {
    'e2e_fairytale': 0.61,
    'e2e_quasar_facts': 0.48,
    'e2e_quasar_sinsig_music': 0.7,
    'ue2e_tv': 0.6,

    'ue2e_general': 0.58,
    'ue2e_navi_auto': 0.70,
    'ue2e_quasar': 0.72050,
}

# Стат.значимость, считается только по этим группам метрик
SIGNIFICANT_METRIC_GROUPS = ['quality', 'error', 'download_error_percent', 'download_error_absolute']

# Метрики, которые по-умолчанию скрываются в html viewer, нужно включать галочку, чтобы увидеть метрики из группы
# Но если будет прокрашенная метрика внутри этих групп — то группы будут сразу показаны
HIDDEN_GROUPS = [
    'download_error_absolute',
    'download_error_details',
    'download_error_percent',
    'asr',
    'diff',
    'nlg',
    'tts',
]

# Помимо групп, стат.значимые прокраски:
SIGNIFICANT_METRIC_NAMES = [
    # метрики качества ASR:
    'wer', 'werp',

    # метрики качества на срезе изменившегося ASR, классификации
    'integral_on_asr_changed', 'integral_on_scenario_changed',

    # качество синтеза речи
    'pser_binary',
]

# Список метрик, где больше — хуже (помимо метрик ошибок)
LESS_IS_BETTER_METRIC_NAMES = ['wer', 'werp', 'pser_binary']


def get_metric_for_same_basket(metrics_item, all_metrics, metric_name):
    """
    В списке метрик `all_metrics` ищет метрику с названием `metric_name` а такой же корзинкой, как и `metrics_item.basket`
    Возвращает массив с метриками, подходящими под фильтр
    По идее должен возвращаться массив только с 1 объектом
    :param dict metrics_item:
    :param list[dict] all_metrics:
    :param str metric_name:
    :return list[dict]:
    """
    if 'basket' not in metrics_item or not all_metrics:
        return []

    return [x for x in all_metrics
            if x.get('basket') == metrics_item['basket'] and x.get('metric_name') == metric_name]


def get_basket_queries_count(metrics_item, all_metrics):
    """
    Возвращает число запросов в корзинке `metrics_item.basket`
    :param dict metrics_item:
    :param list[dict] all_metrics:
    :return Optional[int]:
    """
    result = get_metric_for_same_basket(metrics_item, all_metrics, 'queries_count')
    if len(result) != 1:
        return None
    return result[0]['prod_quality']


def is_significant(item, all_metrics=None):
    """
    Определяет стат. значимость метрики
    :param dict item: json со значениями и другими параметрами метрики
    :param list[dict] all_metrics:
    :return Optional[str]:
    """
    if item.get('pvalue') is None:
        return None

    if item.get('pvalue') <= 0.01:
        return 'level_001'

    queries_count = get_basket_queries_count(item, all_metrics)
    if queries_count is None:
        # по умолчанию стандартный порог 0.03
        return 'level_003' if item.get('pvalue') <= 0.03 else None

    if queries_count >= 5000:
        # для больших корзинок должна выполниться только проверка на 0.01
        return None

    if queries_count <= 1700:
        # для маленьких корзинок повышаем порог pvalue до 0.05
        return 'level_005' if item.get('pvalue') <= 0.05 else None

    # по умолчанию для корзинок 1.7к-5к порог остаётся
    if item.get('pvalue') <= 0.03:
        return 'level_003'

    return None


def is_significant_error_condition(item, thr):
    """
    Определяет значима ли ошибка при определённом пороге thr:
    test >= thr или prod >= thr

    :param dict item: объект со значениями метрики test_quality, prod_quality
    :param float thr: порог, выше которого должна быть метрика ошибки, чтобы быть значимоей
    :return:
    """
    return (
        item.get('test_quality', 0) >= thr or item.get('prod_quality', 0) >= thr
    )


def is_bad_wonderlogs(item):
    """
    Проверяет метрики wonderlogs
    :param dict item:
    :return: bool
    """

    WONDERLOGS_ROWS = 'wonderlogs_rows'
    WONDERLOGS_WEIGHTS = 'wonderlogs_weights'
    DIFFERENT_WONDERLOGS_ROWS = 'different_wonderlogs_rows'

    thresholds = {
        WONDERLOGS_ROWS: {
            'min': 95,
            'max': 105
        },
        WONDERLOGS_WEIGHTS: {
            'min': 90,
            'max': 110
        },
        DIFFERENT_WONDERLOGS_ROWS: {
            'max_percent': 1.6
        }
    }
    metric_name = item.get('metric_name')
    wonderlogs_metrics_list = [WONDERLOGS_ROWS, WONDERLOGS_WEIGHTS, DIFFERENT_WONDERLOGS_ROWS]

    if metric_name not in wonderlogs_metrics_list:
        return False

    metrics_group = item.get('metrics_group')

    if metrics_group == 'download_error_percent':
        diff_percent = item.get('diff_percent', 0)
        return diff_percent > thresholds[metric_name]['max_percent']
    elif metrics_group != 'download_error_absolute':
        return False

    if item.get('test_quality', 0) == 0:
        return True

    diff = 100.0 * item.get('prod_quality', 0) / item.get('test_quality')
    return diff > thresholds[metric_name]['max'] or diff < thresholds[metric_name]['min']


def is_significant_error(item):
    """
    Для json'а с метрикой, возвращает, значима ли ошибка
        т.е. будет ли подсвечиваться строка с ошибкой в html viewer'е и будет ли падать граф на такой ошибке
    Для ошибок, значимость определяется порогом на ошибки а не статистическим тестом
    Для wonderlogs проверяет различие на более чем 5% прод против теста
    :param dict item:
    :return bool:
    """
    if is_bad_wonderlogs(item):
        return True

    if item.get('metrics_group') and item.get('metrics_group') not in ['error', 'download_error_percent', 'tts']:
        return False

    metric_name = item.get('metric_name', '')
    basket = item.get('basket')

    if basket is not None and basket in ERROR_THRESHHOLDS_BY_BASKET:
        # переопределение порога на ошибки по корзинкам
        threshhold = ERROR_THRESHHOLDS_BY_BASKET.get(basket, {}).get(metric_name)
        if threshhold is not None:
            return is_significant_error_condition(item, threshhold)

    if ERROR_THRESHHOLDS.get(metric_name):
        # пороги на ошибки, перечисленные в ERROR_THRESHHOLDS для всех корзинок
        return is_significant_error_condition(item, ERROR_THRESHHOLDS.get(metric_name))

    if 'error' in metric_name:
        # любая остальная ошибка с текстом "error"
        return is_significant_error_condition(item, OTHER_ERROR_THRESHHOLD)

    return False


def get_metric_description(item):
    if item.get('metric_name') == 'all_download_error':
        if item.get('metrics_group') == 'download_error_absolute':
            return 'Суммарное число ошибок прокачки\n' \
                   'Число ошибок прокачки по типам смотрите в группе метрик download_error_absolute\n' \
                   'Подробные ошибки прокачки и сетрейсы смотрите в группе метрик download_error_details'
        else:
            return 'Доля всех ошибок прокачки относительно числа запросов в корзинке\n' \
                   'Допустимо ошибок максимум 1.5% ошибок\n' \
                   'Число ошибок прокачки по типам смотрите в группе метрик download_error_absolute\n' \
                   'Подробные ошибки прокачки и сетрейсы смотрите в группе метрик download_error_details'

    METRICS_DESCRIPTION = {
        'uniproxy_error': 'Ошибки Uniproxy',
        'mm_unanswer_error': 'Ошибки MegaMind',
        'downloader_error': 'Ошибки самой "прокачки" (компонента Алисы в SOY, которая делает запросы к uniproxy)',
        'unknown_error': 'Неизвестная ошибка. Не удалось распарсить ответ прокачки',
        'tunneler_error': 'Ошибки поисковых источников, про которые репортит tunneler',
        'vins_sources_error': 'Ошибки источников MegaMind. Не всегда являются неответом Алисы.\n'
                              'Допустимо 7.5% в прокачке, но при повышенном количестве требуют разбора',

        'wonderlogs_rows': 'Число строк в wonderlog\'ах, сформированных по результатам прокачки\n'
                           'Должно примерно совпадать в PROD и TEST, допустимо различие < 5%',
        'wonderlogs_weights': 'Объём wonderlog\'ов, сформированных по результатам прокачки\n'
                           'Должно примерно совпадать в PROD и TEST, допустимо различие < 10%',
        'different_wonderlogs_rows': 'Число различающихся строк в wonderlogах, сформированных по результатам прокачки',

        'total_queries': 'Число запросов, пришедших в кубик Толоки ue2e Relevance',
        'reused_queries': 'Число запросов, взятых повторно из кеша в reuse оценок внутри кубика Толоки ue2e Relevance',
        'toloka_marked_queries': 'Число запросов, который были размечены Толокерами',
        'screenshots_toloka_marked_queries': 'Число запросов, размеченных в Толочном проекте: ПП (со скриншотами)',
        'navi_toloka_marked_queries': 'Число запросов, размеченных в Толочном проекте: Навигатор',
        'music_toloka_marked_queries': 'Число запросов, размеченных в Толочном проекте: Музыка',
        'conversation_toloka_marked_queries': 'Число запросов, размеченных в Толочном проекте: Болталка',
        'alarm_toloka_marked_queries': 'Число запросов, размеченных в Толочном проекте: Будильники-Таймеры',
        'translate_toloka_marked_queries': 'Число запросов, размеченных в Толочном проекте: Переводчик',
        'facts_toloka_marked_queries': 'Число запросов, размеченных в Толочном проекте: Факты',
        'video_toloka_marked_queries': 'Число запросов, размеченных в Толочном проекте: Видео',
        'main_toloka_marked_queries': 'Число запросов, размеченных в Толочном проекте: Основной (для всех остальных)',
    }

    return METRICS_DESCRIPTION.get(item.get('metric_name'))


def prepare_data(data, eps=0.0001):
    """
    Дополняет недостающими и параметрами по умолчанию
    Не меняет данные, а возвращает новый массив
    :param List[dict] data: json с метриками
    :param float eps:
    :return List[dict]:
    """
    results = []
    for idx, source_item in enumerate(data):
        item = deepcopy(source_item)
        item['idx'] = idx

        # diff
        if item.get('diff') is None and item.get('prod_quality') is not None and item.get('test_quality') is not None:
            item['diff'] = item['test_quality'] - item['prod_quality']

        # diff %
        if item.get('diff_percent') is None and item.get('diff') is not None and item.get('prod_quality') is not None:
            if abs(item['prod_quality']) > eps:
                item['diff_percent'] = 100.0 * item['diff'] / item['prod_quality']
            elif abs(item['diff']) > eps:
                item['diff_percent'] = 100.0
            else:
                item['diff_percent'] = 0

        # заполнение metrics_group по умолчанию
        if item.get('metrics_group') is None:
            if 'error' in item.get('metric_name', ''):
                item['metrics_group'] = 'error'
            else:
                item['metrics_group'] = 'quality'

        # ошибки
        if is_significant_error(item):
            item['significant_state'] = 'error'

        # стат. значимость
        significant_state = is_significant(item, data)
        if significant_state is not None:
            item['significant_state'] = significant_state

        worsen = is_worse(item, data)
        if worsen is not None:
            item['is_worse'] = worsen

        # заполняем описание метрики
        if item.get('metric_description') is None:
            description = get_metric_description(item)
            if description:
                item['metric_description'] = description

        results.append(item)
    return results


def get_baskets_list(data):
    return sorted(list(set([x.get('basket') for x in data if x.get('basket') is not None])))


def get_metrics_groups_list(data, significantly_worse):
    """
    Возвращает список групп метрик,
        также возвращает флаг visible — будет ли группа показана по умолчанию
    :param List[dict] data:
    :param List[dict] significantly_worse:
    :return List[dict]:
    """
    groups_with_any_significantly_worse_metric = set([x.get('metrics_group')
                                                      for x in significantly_worse
                                                      if x.get('metrics_group') is not None])
    result = []
    for metrics_group in set([x.get('metrics_group') for x in data if x.get('metrics_group') is not None]):
        result.append({
            'name': metrics_group,
            'visible': metrics_group not in HIDDEN_GROUPS or metrics_group in groups_with_any_significantly_worse_metric
        })
    return sorted(result, key=lambda x: x['name'])


def get_reused_marks_ratio_metric(metrics_item, all_metrics):
    """Возвращает метрику оценённости из списка всех метрик `all_metrics` для данной корзинки `metrics_item.basket`"""
    return get_metric_for_same_basket(metrics_item, all_metrics, 'reused_marks_ratio')


def is_enough_reused_marks_level(metric_item, where='prod_quality'):
    """Возвращает True или False — достаточный ли уровень оценённости, чтобы считать метрику качества"""
    if len(metric_item) != 1:
        return True

    if where not in metric_item[0]:
        return True

    MARKS_LEVEL = 0.99
    return metric_item[0][where] >= MARKS_LEVEL


def has_enough_reused_marks(metrics_item, all_metrics):
    """
    Возвращает — достаточная ли оценённость (наличие толочных оценок) для уверенного расчёта метрики
    :param dict metrics_item:
    :param list[dict] all_metrics:
    :return bool: - False в случае если для этой же корзинки есть метрика reused_marks_ratio и она меньше порога
    """
    if not metrics_item.get('basket'):
        return True

    if metrics_item.get('metrics_group', '') not in ['quality', 'quality-info'] and \
            not metrics_item.get('metric_name', '').startswith('integral'):
        # Достаточная оценённость нужна только для метрик качества, считающихся по толочным оценкам
        return True

    reused_marks_ratio = get_reused_marks_ratio_metric(metrics_item, all_metrics)
    return (
        is_enough_reused_marks_level(reused_marks_ratio, 'prod_quality')
        and is_enough_reused_marks_level(reused_marks_ratio, 'test_quality')
    )


def has_critical_low_reused_marks_ratio(item):
    """Возвращает True в случае оценённости в тестовой ветке <80% на основных корзинках
    при высокой оценённости в продовой ветке"""
    if item.get('metric_name') != 'reused_marks_ratio':
        return False

    basket = item.get('basket')
    if not basket:
        return False

    if basket not in BASKETS_MIN_PROD_QUALITY_VALUE:
        return False

    prod_reuse, test_reuse = item.get('prod_quality'), item.get('test_quality')
    if prod_reuse is None or test_reuse is None:
        return False

    REUSED_MARKS_MINIMUM_LEVEL = 0.8
    return test_reuse < REUSED_MARKS_MINIMUM_LEVEL and prod_reuse > REUSED_MARKS_MINIMUM_LEVEL


def is_bad_quality_for_input_basket(item, thr=0.7):
    """
    Проверяет абсолютные значение метрики integral у кастомных корзинок
    :param dict item:
    :param float thr: порог, ниже которого корзина должна быть прокрашена
    :return: bool
    """
    if item.get('basket') != 'input_basket':
        return False

    if item.get('metric_name') != 'integral':
        return False

    if item.get('test_quality') is None:
        return False

    return item.get('test_quality') < thr


def is_bad_production_quality(item, all_metrics):
    """Возвращает True в случае низкого значения метрики integral в prod ветке в конкретных фиксированных корзинках"""
    basket = item.get('basket')
    if not basket:
        return False

    if basket not in BASKETS_MIN_PROD_QUALITY_VALUE:
        return False

    if item.get('metric_name') != 'integral':
        return False

    reused_marks_ratio = get_reused_marks_ratio_metric(item, all_metrics)
    if is_enough_reused_marks_level(reused_marks_ratio, 'prod_quality') is False:
        return False

    prod_quality = item.get('prod_quality')
    if prod_quality is None:
        return False

    return prod_quality < BASKETS_MIN_PROD_QUALITY_VALUE[basket]


def has_enough_count_on_slice_metrics(item, metrics_list):
    if item.get('metric_name', '') == 'integral_on_asr_changed':
        count_on_asr_changed = get_metric_for_same_basket(item, metrics_list, 'queries_count_on_asr_changed')
        if len(count_on_asr_changed) > 0:
            return count_on_asr_changed[0].get('diff', 0) > 30
    if item.get('metric_name', '') == 'integral_on_scenario_changed':
        count_on_scenario_changed = get_metric_for_same_basket(item, metrics_list, 'queries_count_on_scenario_changed')
        if len(count_on_scenario_changed) > 0:
            return count_on_scenario_changed[0].get('diff', 0) > 30
    return True


def is_worse(item, metrics_list):
    """
    Проверяет надо ли прокрашивать метрику
    :param dict item:
    :param list[dict] metrics_list:
    :param Optional[list[str]] significant_metric_groups:
    :return: bool
    """

    if is_significant_error(item):
        return True

    if item.get('metrics_group', '') == 'download_error_absolute':
        return None
    if has_critical_low_reused_marks_ratio(item):
        item['significant_state'] = 'low_reused_marks_ratio'
        return True
    if not has_enough_reused_marks(item, metrics_list):
        return None
    if not has_enough_count_on_slice_metrics(item, metrics_list):
        return None
    if is_bad_production_quality(item, metrics_list):
        # недостаточное качество в продовой ветке — роняем приёмку
        item['significant_state'] = 'bad_prod_quality'
        return True
    if is_bad_quality_for_input_basket(item):
        # При маленьком абсолютном значении метрики всегда прокрашиваем
        item['significant_state'] = 'low_quality'
        return True
    if is_significant(item, metrics_list) is not None:
        if item.get('diff') is not None:
            diff = item['diff']
        elif item.get('test_quality') is not None and item.get('prod_quality') is not None:
            diff = item['test_quality'] - item['prod_quality']
        else:
            return None
        return diff > 0 if item.get('metric_name', '') in LESS_IS_BETTER_METRIC_NAMES else diff < 0
    return None


def filter_significant_results(metrics_list, significant_metric_groups=None):
    """
    Возвращает стат.значимо прокрашенные метрики: улучшения и ухудшения
    :param list[dict] metrics_list:
    :param Optional[list[str]] significant_metric_groups:
    :return tuple[list[dict], list[dict]]:
    """
    significantly_better = []
    significantly_worse = []

    if significant_metric_groups is None:
        significant_metric_groups = SIGNIFICANT_METRIC_GROUPS

    for item in metrics_list:
        if 'metrics_group' in item and item['metrics_group'] not in significant_metric_groups \
                and item.get('metric_name', '') not in SIGNIFICANT_METRIC_NAMES:
            continue
        worsen = is_worse(item, metrics_list)
        if worsen is None:
            continue
        if worsen:
            significantly_worse.append(item)
        else:
            significantly_better.append(item)
    return significantly_better, significantly_worse


def get_nirvana_meta():
    import nirvana.job_context as nv
    import utils.nirvana.api as nirvana_api

    ctx = nv.context()
    instance_id = ctx.get_meta().get_workflow_instance_uid()

    meta_data = nirvana_api.get_instance_meta(instance_id)
    global_params = nirvana_api.get_global_params(instance_id)

    return meta_data, global_params


def prepare_nirvana_meta(meta_data, global_params):
    return {
        'instance_id': meta_data.get('instanceId'),
        'instance_name': meta_data.get('name', ''),
        'instance_description': meta_data.get('instanceComment', ''),
        'instance_author': meta_data.get('instanceCreator', ''),
        'instance_started': meta_data.get('started', ''),
        'instance_cloned_from': meta_data.get('cloneOfInstance'),

        'param_abc_id': global_params.get('abc_id'),
        'param_priority': global_params.get('priority'),
        'param_startrek_ticket': global_params.get('startrek_ticket', ''),

        'param_prod_url': global_params.get('prod_url'),
        'param_prod_uniproxy_url': global_params.get('prod_uniproxy_url'),
        'param_prod_experiments': global_params.get('prod_experiments', []),
        'param_test_url': global_params.get('test_url'),
        'param_test_uniproxy_url': global_params.get('test_uniproxy_url'),
        'param_test_experiments': global_params.get('test_experiments', []),
    }


def get_favicon_base64(is_ok, template_folder):
    filename = 'favicon_ok.txt' if is_ok else 'favicon_notok.txt'
    with open(os.path.join(template_folder, filename)) as f:
        return f.read()


def render_metrics_table(source_data, show_metadata=False, template_folder='operations/priemka/metrics_viewer',
                         significant_metric_groups=None):
    if significant_metric_groups is None:
        significant_metric_groups = SIGNIFICANT_METRIC_GROUPS

    with open(os.path.join(template_folder, 'template.html')) as f:
        TEMPLATE = f.read()

    data = prepare_data(source_data)
    significantly_better, significantly_worse = filter_significant_results(
        data,
        significant_metric_groups=significant_metric_groups
    )

    if show_metadata:
        meta_data, global_params = get_nirvana_meta()
    else:
        meta_data, global_params = {}, {}
    template_data = prepare_nirvana_meta(meta_data, global_params)
    template_data.update(dict(
        SHOW_METADATA='' if show_metadata else 'hidden',
        SOURCE_DATA=json.dumps(data),
        SIGNIFICANT_METRIC_GROUPS=significant_metric_groups,
        SIGNIFICANT_METRIC_NAMES=SIGNIFICANT_METRIC_NAMES,
        BASKETS_LIST=str(get_baskets_list(data)),
        METRICS_GROUPS_LIST=json.dumps(get_metrics_groups_list(data, significantly_worse)),
        FAVICON=get_favicon_base64(len(significantly_worse) == 0, template_folder)
    ))

    return Template(TEMPLATE).substitute(template_data)
