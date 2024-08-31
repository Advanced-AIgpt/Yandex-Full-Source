# -*- coding: utf-8 -*-
"""
Работа с аргументами
"""
from argparse import ArgumentParser, Namespace
from typing import Tuple, Dict, List

ARGS: Dict = {
    "f_alerts": "(-A|--alerts)",
    "f_dashboard": "(-D|--dashboard)",
    "f_wipe": "(-W|--wipe)",
    "f_verify": "(-V|--verify)",
    "f_verbose": "(-v|--verbose)",
    "f_warning": "(-w|--warning)",
    "f_diff": "(-o|--show-diff)",
    "f_log": "(-l|--log)",
    "parallel_count": "(-p|--parallel-count)",
    "f_dry_run": "(-d|--dry-run)",
    "f_do_not_delete": "(-x|--do-not-delete)",
    "config_dir": "(-c|--config-dir)",
    "alerts_dir": "(-i|--alerts-dir)",
    "save_alert_dir": "(-s|--save-alerts)",
    "target": "(-T|--target)",
    "dashboard_json": "(-j|--dashboard-json)",
    "save_dashboard_json": "(-a|--save-dashboard-json)",
    "format_json": "(-f|--format)",
    "yasm_key": "(-k|--yasm-key)",
    "yasm_title": "(-t|--yasm-title)",
    "yasm_user": "(-u|--yasm-user)",
    "yasm_editors": "(-e|--yasm-editors)",
}
USAGE: str = f"""
  yasm-client [-v|-w] [-d|-x] -W -T TARGET [-p PARALLEL_COUNT] [-l LOGFILE]
  yasm-client [-v|-w] [-d|-x] -A (-c CONFIG_DIR | -i ALERTS_DIR) [-o] [-l LOGFILE]
  yasm-client [-v|-w] [-d] -D (-A | -j DASHBOARD_JSON) [-f FORMAT_JSON] [-k YASM_KEY] [-t YASM_TITLE] [-u YASM_USER] [-l LOGFILE]
              [-e YASM_EDITOR [-e YASM_EDITOR] ...] [-l LOGFILE]
  yasm-client -V -c CONFIG_DIR [-l LOGFILE]
arguments:
  {ARGS["f_alerts"]:28}Генерация и применение алертов. Алерты применяются автоматически при мердже в транк.
  {ARGS["f_dashboard"]:28}Генерация и применение панели
  {ARGS["f_wipe"]:28}Удаление алертов
  {ARGS["f_verify"]:28}Проверка директории с конфигами
  {ARGS["f_verbose"]:28}Подробный вывод
  {ARGS["f_warning"]:28}Менее подробный вывод
  {ARGS["f_diff"]:28}Выводить diff для алертов без verbose режима
  {ARGS["f_log"]:28}Перенаправить вывод в лог файл
  {ARGS["parallel_count"]:28}Количество параллельных запросов (15)
  {ARGS["f_dry_run"]:28}Не применять изменения
  {ARGS["f_do_not_delete"]:28}Не удалять
  {ARGS["config_dir"]:28}Директория с конфигами для генерации алертов
  {ARGS["alerts_dir"]:28}Директория со сгенерированными JSON файлами алертов
  {ARGS["save_alert_dir"]:28}Директория для сохранения JSON файлов алертов
  {ARGS["target"]:28}Патерн имен алертов для удаления
  {ARGS["dashboard_json"]:28}Файл с описанием панели
  {ARGS["save_dashboard_json"]:28}Файл для сохранения панели
  {ARGS["format_json"]:28}Файл с дополнительным форматирования панели
  {ARGS["yasm_key"]:28}Ключ панели
  {ARGS["yasm_title"]:28}Заголовок панели
  {ARGS["yasm_user"]:28}Владелец панели
  {ARGS["yasm_editors"]:28}Редактор панели
"""


# -------------------------------------------------------------------------------------------------
def msg() -> str:
    """
    Вывод справки по аргументам

    :return: Строка справки
    :rtype: str
    """
    return USAGE


# -------------------------------------------------------------------------------------------------
def check_arguments(nms: Namespace) -> Tuple[bool, str]:
    """
    Проверка корректности и совместимости переданных аргументов

    :param nms: argparse нэймспейс
    :type nms: Namespace
    :return: Флаг корректности и строка с сообщением об ошибке
    :rtype: Tuple[bool, str]
    """
    result: bool = True
    message: str = ""
    if nms.f_warning and nms.f_verbose:
        result = False
        message = (
            f'Нельзя одновременно установить DEBUG {ARGS["f_verbose"]} и'
            + f' WARNING {ARGS["f_warning"]} уровни логирования'
        )
    elif nms.f_wipe and (nms.f_alerts or nms.f_dashboard):
        result = False
        message = (
            f'Нельзя одновременно создавать алеры {ARGS["f_alerts"]} и/или'
            + f'дашборд {ARGS["f_dashboard"]} и удалять {ARGS["f_wipe"]} алерты'
        )
    elif nms.f_wipe and not nms.target:
        result = False
        message = f'Не указан патерн {ARGS["target"]} для удаления {ARGS["f_wipe"]}'
    elif nms.f_verify and nms.f_wipe:
        result = False
        message = f'Проверка {ARGS["f_verify"]} и удаление {ARGS["f_wipe"]} несовместны'
    elif nms.f_verify and not nms.config_dir:
        result = False
        message = f'Для проверки {ARGS["f_verify"]} требуется указать директорию с конфигами {ARGS["config_dir"]}'
    elif all([nms.f_dashboard, nms.f_dry_run, not nms.save_dashboard_json]):
        result = False
        message = (
            f'В режиме {ARGS["f_dry_run"]} для панели'
            + f'{ARGS["f_dashboard"]} не указан файл для'
            + f' сохранения {ARGS["save_dashboard_json"]}'
        )
    elif nms.f_dashboard and nms.f_do_not_delete:
        result = False
        message = f'Для панели {ARGS["f_dashboard"]} указан ключ {ARGS["f_do_not_delete"]}'
    elif all([nms.f_dashboard, not nms.config_dir, not nms.dashboard_json]):
        result = False
        message = (
            f'Не указан источник для панели {ARGS["f_dashboard"]}.'
            + f' Либо сгенерируйте алерты {ARGS["f_alerts"]} и {ARGS["config_dir"]},'
            + f' либо укажите JSON файл панели {ARGS["dashboard_json"]}'
        )
    elif all([nms.f_dashboard, not nms.dashboard_json, not nms.yasm_key]):
        result = False
        message = f'Для панели {ARGS["f_dashboard"]} не указан ключ {ARGS["yasm_key"]}'
    elif all([nms.f_dashboard, not nms.dashboard_json, not nms.yasm_user]):
        result = False
        message = f'Для панели {ARGS["f_dashboard"]} не указан автор {ARGS["yasm_user"]}'
    elif not nms.f_dashboard and any([nms.format_json, nms.yasm_key, nms.yasm_title, nms.yasm_editors, nms.yasm_user]):
        result = False
        message = f'Аргументы панели указаны без {ARGS["f_dashboard"]}'
    elif nms.f_diff and not nms.f_alerts:
        result = False
        message = f'Ключ {ARGS["f_diff"]} не имеет смысла без {ARGS["f_alerts"]}'
    elif all([nms.f_dashboard, nms.dashboard_json, nms.save_dashboard_json]):
        result = False
        message = (
            f'Источник {ARGS["dashboard_json"]} и назначение'
            + f' {ARGS["save_dashboard_json"]} - JSON файл. Воспользуйтесь копированием.'
        )
    elif all([nms.f_alerts, not nms.config_dir, not nms.alerts_dir]):
        result = False
        message = (
            f'Не указан источник для алертов {ARGS["f_alerts"]}. Укажите либо'
            + f' директорию с конфигами {ARGS["config_dir"]},'
            + f' либо директорию с алертами {ARGS["alerts_dir"]}'
        )
    elif all([nms.f_alerts, nms.config_dir, nms.alerts_dir]):
        result = False
        message = (
            f'Следует указывать либо директорию с конфигами {ARGS["config_dir"]},'
            + f' либо директорию с алертами {ARGS["alerts_dir"]},'
            + " но не оба аргумента одновременно"
        )
    elif all([nms.f_alerts, nms.alerts_dir, nms.save_alert_dir]):
        result = False
        message = (
            f'Источник {ARGS["alerts_dir"]} и назначение {ARGS["save_alert_dir"]}'
            + " - директории с JSON файлами алертов. Воспользуйтесь копированием."
        )
    elif nms.f_dry_run and nms.f_do_not_delete:
        result = False
        message = f'Флаг {ARGS["f_do_not_delete"]} является подмножеством {ARGS["f_dry_run"]}'
    return result, message


# --------------------------------------------------------------------------------------------------
def get_arguments(arguments: List) -> Dict:
    """
    Аргументы коммандной строки

    :param arguments: Список аргументов
    :type arguments: List
    :return: Словарь с аргументами вызова
    :rtype: Dict
    """
    parser: ArgumentParser = ArgumentParser(usage=msg())
    # Общие аргументы
    parser.add_argument("-l", "--log", dest="f_log", type=str, required=False)
    parser.add_argument("-d", "--dry-run", dest="f_dry_run", action="store_true", required=False)
    parser.add_argument(
        "-x",
        "--do-not-delete",
        dest="f_do_not_delete",
        action="store_true",
        default=False,
        required=False,
    )
    parser.add_argument("-p", "--parallel-count", dest="parallel_count", type=int, default=15, required=False)
    parser.add_argument("-v", "--verbose", dest="f_verbose", action="store_true", default=False, required=False)
    parser.add_argument("-w", "--warning", dest="f_warning", action="store_true", default=False, required=False)
    # Аргументы генерации алертов
    parser.add_argument("-A", "--alerts", dest="f_alerts", action="store_true", required=False)
    parser.add_argument("-s", "--save-alerts", dest="save_alert_dir", type=str, required=False)
    parser.add_argument("-c", "--config-dir", dest="config_dir", required=False)
    parser.add_argument("-i", "--alerts-dir", dest="alerts_dir", type=str, required=False)
    parser.add_argument("-o", "--show-diff", dest="f_diff", action="store_true", required=False)
    # Аргументы для удаления алертов
    parser.add_argument("-W", "--wipe", dest="f_wipe", action="store_true", required=False)
    parser.add_argument("-T", "--target", dest="target", type=str, required=False)
    # Аргументы генерации дашбордов
    parser.add_argument("-D", "--dashboard", dest="f_dashboard", action="store_true", required=False)
    parser.add_argument("-j", "--dashboard-json", dest="dashboard_json", type=str, required=False)
    parser.add_argument("-a", "--save-dashboard-json", dest="save_dashboard_json", type=str, required=False)
    parser.add_argument("-f", "--format", type=str, dest="format_json", required=False)
    parser.add_argument("-k", "--yasm-key", dest="yasm_key", type=str, required=False)
    parser.add_argument("-t", "--yasm-title", dest="yasm_title", type=str, required=False)
    parser.add_argument("-u", "--yasm-user", dest="yasm_user", type=str, required=False)
    parser.add_argument("-e", "--yasm-editors", action="append", dest="yasm_editors", type=str, required=False)
    parser.add_argument("-V", "--verify", dest="f_verify", action="store_true", required=False)
    args: Namespace = parser.parse_args(arguments)
    result, message = check_arguments(args)
    if not result:
        parser.error(message)
        return {}
    return vars(args)
