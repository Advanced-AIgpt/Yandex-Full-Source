# coding: utf-8

import click
import logging

from alice.analytics.operations.priemka.alice_parser.lib.alice_parser import AliceParser
from alice.analytics.operations.priemka.alice_parser.utils.yql_utils import get_yql_share_url
from alice.analytics.operations.priemka.alice_parser.utils.files_utils import (
    get_file_content,
    get_table_from_nirvana_input,
    save_table_to_nirvana_output,
    save_content_to_file,
)


def _setup_logger(debug_log):
    logger = logging.getLogger('')
    logger.setLevel(logging.DEBUG)

    log_format = '[%(levelname)s] %(asctime)s - %(message)s'
    log_formatter = logging.Formatter(log_format)

    console = logging.StreamHandler()
    console.setLevel(logging.INFO)
    console.setFormatter(log_formatter)
    logger.addHandler(console)

    if debug_log:
        file_handler = logging.FileHandler(debug_log, mode='w')
        file_handler.setLevel(logging.DEBUG)
        file_handler.setFormatter(log_formatter)
        logger.addHandler(file_handler)


@click.command()
@click.option('--basket_table', help='YT таблица с корзинкой запросов')
@click.option('--basket_table_file', help='Путь к файлу в нирване с MR-table с путём к YT таблице с корзинкой запросов')
@click.option('--downloader_result_table', help='YT таблица с результатом прокачки')
@click.option('--downloader_result_table_file',
              help='Путь к файлу в нирване с MR-table с путём к YT таблице с результатом прокачки')
@click.option('--url_table', help='YT таблица с урла-ми скриншотов')
@click.option('--url_table_file',
              help='Путь к файлу в нирване с MR-table с путём к YT таблице с урла-ми скриншотов')
@click.option('--result_table', help='YT таблица с результатами для всех запросов')
@click.option('--result_table_file',
              help='Путь к файлу в нирване, куда нужно записать MR-table с результатами для всех запросов')
@click.option('--toloka_tasks_table', help='YT таблица с заданиями для толоки')
@click.option('--toloka_tasks_table_file',
              help='Путь к файлу в нирване, куда нужно записать MR-table с заданиями для толоки')
@click.option('--cluster', default='hahn', help='YT кластер')
@click.option('--token', help='YT token')
@click.option('--token_file', help='Файл, содержащий YT token')
@click.option('--udf_path', default='udf/libalice_parser.so',
              help='Относительный путь к udf so-шке для запуска nile-over-yql')
@click.option('--store_checkpoints', is_flag=True, default=False,
              help='Включает режим чекпоинтов для nile: пишет промежуточные таблицы')
@click.option('--need_voice_url', is_flag=True, default=False,
              help='Нужно ли в результаты Толоки добавлять ссылку на голос в MDS')
@click.option('--is_dsat', is_flag=True, default=False, help='Прокидывает дополнительные колонки в результат парсинга')
@click.option('--always_voice_text', is_flag=True, default=False,
              help='При парсинге всегда использовать voice_text для формирования answer')
@click.option('--mode', default='quasar', help='Режим парсера')
@click.option('--tables_ttl', default=14, type=int, help='TTL на выходные таблички в YT')
@click.option('--yt_pool', help='Вычислительный пул на YT')
@click.option('--debug_log', type=str, help='Название файла, куда выводить debug.log')
@click.option('--start_date', help='дата старта при парсинге логов')
@click.option('--end_date', help='дата окончания при парсинге логов')
@click.option('--merge_tech_queries', help='нужно ли объединять данные технических запросов')
@click.option('--yql_link_file', multiple=True, help='Файлы, куда сохранить ссылку YQL запрос после выполнения кубика')
def main(
    basket_table,
    basket_table_file,
    cluster,
    token,
    token_file,
    udf_path,
    store_checkpoints=False,
    downloader_result_table=None,
    downloader_result_table_file=None,
    mode='quasar',
    need_voice_url=False,
    is_dsat=False,
    always_voice_text=False,
    toloka_tasks_table=None,
    toloka_tasks_table_file=None,
    result_table=None,
    result_table_file=None,
    url_table=None,
    url_table_file=None,
    tables_ttl=14,
    yt_pool=None,
    debug_log=None,
    start_date=None,
    end_date=None,
    merge_tech_queries=False,
    yql_link_file=None,
):
    _setup_logger(debug_log)

    logging.info('start alice parser')

    if basket_table_file:
        basket_table = get_table_from_nirvana_input(basket_table_file)

    if downloader_result_table_file:
        downloader_result_table = get_table_from_nirvana_input(downloader_result_table_file)

    if url_table_file:
        url_table = get_table_from_nirvana_input(url_table_file)

    if token_file:
        token = get_file_content(token_file)

    assert basket_table, 'Нужно указать --basket_table или --basket_table_file'

    ap = AliceParser(basket_table, downloader_result_table, result_table, toloka_tasks_table,
                     url_table, mode=mode, token=token, tables_ttl=tables_ttl, pool=yt_pool,
                     cluster=cluster, udf_path=udf_path, store_checkpoints=store_checkpoints,
                     start_date=start_date, end_date=end_date, merge_tech_queries=merge_tech_queries,
                     need_voice_url=need_voice_url, is_dsat=is_dsat, always_voice_text=always_voice_text,
                     )

    yql_id = ap.run()

    if yql_link_file:
        yql_link = get_yql_share_url(yql_id)
        for filename in yql_link_file:
            save_content_to_file(filename, yql_link)

    if result_table_file:
        save_table_to_nirvana_output(result_table_file, cluster, result_table)

    if toloka_tasks_table_file:
        save_table_to_nirvana_output(toloka_tasks_table_file, cluster, toloka_tasks_table)
