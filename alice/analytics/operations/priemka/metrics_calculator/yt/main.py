# coding: utf-8

import sys
import click
import logging
from alice.analytics.operations.priemka.metrics_calculator.calc_metrics_yt import calc_metrics
from alice.analytics.operations.priemka.alice_parser.utils.files_utils import (
    get_file_content,
    get_table_from_nirvana_input,
    save_table_to_nirvana_output,
)


def _setup_logger():
    logging.basicConfig(
        format="[%(levelname)s] %(asctime)s - %(message)s",
        stream=sys.stdout,
        level=logging.INFO
    )


@click.command()
@click.option('--input_table', help='YT таблица с результатами прокачки в формате таблицы для пульсара')
@click.option('--input_table_file', help='Путь к файлу нирваны с MR-table с путём к YT таблице с результатами прокачки')
@click.option('--output_table', help='Папка на YT с выходной таблицей с результатами')
@click.option('--output_table_file', help='Путь к файлу в нирване, куда нужно записать MR-table с результатами')
@click.option('--cluster', default='hahn', help='YT кластер')
@click.option('--token', help='YT token')
@click.option('--token_file', help='Файл, содержащий YT token')
@click.option('--yt_pool', help='Вычислительный пул на YT')
@click.option('--udf_path', default='../udf/libalice_metrics_calculator_yt_udf.so', help='Относительный путь к udf so-шке для запуска nile-over-yql')
@click.option('--tables_ttl', default=14, type=int, help='TTL на выходные таблички в YT')
@click.option('--custom_metrics')
def main(
    udf_path,
    input_table=None,
    input_table_file=None,
    output_table=None,
    output_table_file=None,
    cluster=None,
    token=None,
    token_file=None,
    yt_pool=None,
    tables_ttl=None,
    custom_metrics=None
):
    _setup_logger()

    logging.info('start alice_metrics_calculator yt')

    if token_file:
        token = get_file_content(token_file)

    if input_table_file:
        input_table = get_table_from_nirvana_input(input_table_file)

    calc_metrics(input_table, output_table, udf_path, yt_pool, token, tables_ttl, custom_metrics)

    if output_table_file:
        save_table_to_nirvana_output(output_table_file, cluster, output_table)
