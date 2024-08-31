# This script is performing collection of all buckets for Verstehen via Nirvana
import argparse
import logging

import vh

logger = logging.getLogger(__name__)


APP_ID_FILTERING_MAPPING = {
    'all':             '',
    'search_app_prod': 'AND $get_app_id(request) IN ("ru.yandex.searchplugin", "ru.yandex.mobile")',
    'search_app_beta': 'AND $get_app_id(request) IN ("ru.yandex.searchplugin.beta")',
    'browser_prod':    'AND $get_app_id(request) IN ("ru.yandex.mobile.search", "ru.yandex.mobile.search.ipad", '
                       '"com.yandex.browser")',
    'stroka':          'AND $get_app_id(request) IN ("winsearchbar")',
    'browser_alpha':   'AND $get_app_id(request) IN ("com.yandex.browser.alpha")',
    'browser_beta':    'AND $get_app_id(request) IN ("com.yandex.browser.beta")',
    'navigator':       'AND $get_app_id(request) IN ("ru.yandex.yandexnavi")',
    'launcher':        'AND $get_app_id(request) IN ("com.yandex.launcher")',
    'quasar':          'AND $get_app_id(request) IN ("ru.yandex.quasar.services")',
    'auto':            'AND $get_app_id(request) IN ("yandex.auto", "ru.yandex.autolauncher")',
    'elariwatch':      'AND $get_app_id(request) IN ("ru.yandex.iosdk.elariwatch")'
}

LAST_HALF_YEAR = '`2019-07-01`, `2026-12-31`'
WHOLE_TIME = '`2018-01-01`, `2026-12-31`'


def launch_bucket_collection_graph(config):
    config['app_id_filtering'] = APP_ID_FILTERING_MAPPING[config['bucket_name']]

    with vh.Graph() as g:
        bucket_collection_operation = vh.op(name='Verstehen Bucket Preparation', owner='artemkorenev')
        bucket_collection_operation(
            **config
        )

    info = vh.run(g).get_workflow_info()

    link = 'https://nirvana.yandex-team.ru/flow/{}'.format(info.workflow_id)
    logger.debug('Started workflow in url: {}'.format(link))
    return link


if __name__ == '__main__':
    logging.basicConfig(
        format='%(asctime)s %(levelname)s:%(name)s %(message)s',
        level=logging.DEBUG
    )

    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--yql-token', required=True, help='YQL Secret to pass to Nirvana'
    )
    parser.add_argument(
        '--yt-token', required=True, help='YT Secret to pass to Nirvana'
    )
    parser.add_argument(
        '--sandbox-token', required=True, help='Sandbox Secret to pass to Nirvana'
    )
    parser.add_argument(
        '--sandbox-user', required=True, help='Sandbox user to pass to Nirvana'
    )
    args = parser.parse_args()

    configs = [
        {
            'bucket_name': 'all',
            'logs_date_range': LAST_HALF_YEAR,
        },
        {
            'bucket_name': 'search_app_prod',
            'logs_date_range': LAST_HALF_YEAR,
        },
        {
            'bucket_name': 'search_app_beta',
            'logs_date_range': LAST_HALF_YEAR,
        },
        {
            'bucket_name': 'browser_prod',
            'logs_date_range': LAST_HALF_YEAR,
        },
        {
            'bucket_name': 'stroka',
            'logs_date_range': LAST_HALF_YEAR,
        },
        {
            'bucket_name': 'browser_alpha',
            'logs_date_range': LAST_HALF_YEAR,
        },
        {
            'bucket_name': 'browser_beta',
            'logs_date_range': WHOLE_TIME,
        },
        {
            'bucket_name': 'navigator',
            'logs_date_range': WHOLE_TIME,
        },
        {
            'bucket_name': 'launcher',
            'logs_date_range': WHOLE_TIME,
        },
        {
            'bucket_name': 'quasar',
            'logs_date_range': WHOLE_TIME,
        },
        {
            'bucket_name': 'auto',
            'logs_date_range': WHOLE_TIME,
        },
        {
            'bucket_name': 'elariwatch',
            'logs_date_range': WHOLE_TIME,
        }
    ]

    args_dict = vars(args)

    links = []
    for config in configs:
        config.update(args_dict)
        links.append(launch_bucket_collection_graph(config))

    logger.debug('Started Nirvana workflows:')
    for link in links:
        logger.debug(link)
