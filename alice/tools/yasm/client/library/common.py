"""
Общие функции для генератора алертов
"""
from typing import Dict, List, Tuple, Any
from csv import reader as csv_reader
from json import loads as j_loads
from logging import Logger, getLogger
from errno import EEXIST
from os import mkdir
from os.path import isdir, join as os_join, abspath
from re import compile as re_compile, Pattern
from yaml import load as y_load, FullLoader

YASM_API_HOST: str = "https://yasm.yandex-team.ru/srvambry"
LOCATIONS: List = ["man", "sas", "vla"]
HTTP_CODES_TO_RETRY: List = [500, 502, 503, 504]
REPLACE_PAIRS: List = [("-", "_"), (".", "_")]
OBJECT_NAME_TMPL: str = "{prefix}.{itype}{dot}{prj}"
ALERT_NAME_TMPL: str = "{object}-{alert}"
USAGE: str = """
"""


# --------------------------------------------------------------------------------------------------
def gen_full_alert_name(obj: Dict, alert: str) -> str:
    """
    Формирование имени алерта

    :param obj: Объект алертов
    :type obj: Dict
    :param alert: Название алерта
    :type alert: str
    :return: Имя алерта
    :rtype: str
    """
    parts: Dict = dict()
    parts["object"] = obj["name"]
    parts["alert"] = str_replace(alert, REPLACE_PAIRS)
    return ALERT_NAME_TMPL.format(**parts)


# --------------------------------------------------------------------------------------------------
def gen_alert_label(alert_data: Dict, name_prefix: str, loc: str = None) -> Dict:
    """
    Возвращает список словарей с названием и описанием алертов

    :param alert_data: Словарь, описывающий данные алерта
    :type alert_data: Dict
    :param name_prefix: Префикс человекочитаемого имени алерта
    :type name_prefix: str
    :param loc: Локация
    :type loc: str
    :return: Словарь alert + label
    :rtype: Dict
    """
    new_label = dict()
    alert_name: str = alert_data["name"]
    new_label["alert"] = alert_name
    new_label["label"] = name_prefix
    if all([loc, loc != "nodc", "no_locations" not in alert_data]):
        new_label["label"] += f" {loc.upper()}"
    return new_label


# --------------------------------------------------------------------------------------------------
def load_json_file(path: str) -> Tuple[bool, Dict]:
    """
    Возвращает содержимое JSON файла в виде словаря

    :param path: Полное имя JSON файла
    :type path: str
    :return: JSON в виде словаря
    :rtype: Tuple[bool, Dict]
    """
    log: Logger = getLogger("yasm-client")
    result: Dict = dict()
    status: bool = True
    try:
        with open(path) as json_file:
            result = j_loads(json_file.read())
    except IOError as error:
        log.error("Невозможно прочесть файл %s: %s", path, error)
        status = False
    except ValueError as error:
        log.error("Неверный формат JSON (%s): %s", path, error)
        status = False
    return status, result


# --------------------------------------------------------------------------------------------------
def load_yaml_file(path: str) -> Tuple[bool, Dict]:
    """
    Возвращает содержимое YAML файла в виде словаря

    :param path: Полное имя YAML файла
    :type path: str
    :return: JSON в виде словаря
    :rtype: Tuple[bool, Dict]
    """
    log: Logger = getLogger("yasm-client")
    result: Dict = dict()
    status: bool = True
    try:
        with open(path) as yaml_file:
            result = y_load(yaml_file, Loader=FullLoader)
    except IOError as error:
        log.error("Невозможно прочесть файл %s: %s", path, error)
        status = False
    except ValueError as error:
        log.error("Неверный формат YAML (%s): %s", path, error)
        status = False
    return status, result


# --------------------------------------------------------------------------------------------------
def load_csv_file(path: str) -> Tuple[bool, List[Dict]]:
    """
    Возвращает содержимое CSV файла в виде списка кортежей

    :param path: Полное имя CSV файла
    :type path: str
    :return: Список словарей, содержащих озаглавленные поля построчно
    :rtype: Tuple[bool, List[Dict]]
    """
    log: Logger = getLogger("yasm-client")
    result: List[Dict] = list()
    try:
        with open(path) as csv_file:
            csvreader = csv_reader(csv_file)
            fields: List = csvreader.__next__()
            fields_count: int = len(fields)
            for line in csvreader:
                if line:
                    if len(line) != fields_count:
                        log.warning(
                            "Несовпадение числа полей данных в файле %s, строка '%s' (длина %s, должно быть %s).",
                            path,
                            ','.join(line),
                            len(line),
                            fields_count,
                        )
                    result.append(dict(zip(fields, line)))
    except IOError as error:
        log.error("Невозможно прочесть файл %s: %s", path, error)
        return False, [{}]
    return True, result


# --------------------------------------------------------------------------------------------------
def load_objects(path: str) -> Tuple[bool, Dict[str, Any]]:
    """
    Формирует словарь объектов

    :param path: Пусть к object.yml
    :type path: str
    :return: Словарь объектов
    :rtype: Tuple[bool, Dict[str,Any]]
    """

    def gen_name(local_obj: Dict, local_prefix: str, local_itype: str) -> str:
        parts: Dict = dict()
        parts["prefix"] = local_prefix
        parts["itype"] = local_itype
        parts["prj"] = local_obj.get("prj", "")
        if "override" in obj:
            parts["itype"] = local_obj["override"].get("itype", parts["itype"])
            parts["prj"] = local_obj["override"].get("prj", parts["prj"])
        parts["prefix"] = str_replace(parts["prefix"], [("-", "_")])
        for local_key in ["itype", "prj"]:
            parts[local_key] = str_replace(parts[local_key], REPLACE_PAIRS)
        parts["dot"] = "." if parts["prj"] != "" else ""
        return OBJECT_NAME_TMPL.format(**parts)

    objects = dict()
    response, src = load_yaml_file(path)
    if response:
        for prefix in src:
            for itype in src[prefix]:
                for obj in src[prefix][itype]:
                    elem = dict()
                    elem["name"] = gen_name(obj, prefix, itype)
                    elem["prefix"] = prefix
                    elem["itype"] = itype
                    elem["hosts"] = obj.get("hosts", "ASEARCH")
                    if "ctype" in obj:
                        if isinstance(obj["ctype"], list):
                            elem["ctype"] = obj["ctype"].copy()
                        else:
                            elem["ctype"] = obj["ctype"]
                    else:
                        elem["ctype"] = "prod"
                    for key in ["prj", "locations", "no_locations", "desc"]:
                        if key in obj:
                            if isinstance(obj[key], list):
                                elem[key] = obj[key].copy()
                            else:
                                elem[key] = obj[key]
                    objects[elem["name"]] = elem
    return response, objects


# --------------------------------------------------------------------------------------------------
def load_configs(data: Dict) -> bool:
    """
    Загрузка конфигов из директории

    :param data: Данные для загрузки
    :type data: Dict
    :return: Успешно или нет
    :rtype: bool
    """
    log: Logger = getLogger("yasm-client")
    result: bool = True
    if data["config_dir"]:
        log.info("Загружаем конфиг алертов в директории %s", abspath(data["config_dir"]))
        # configs, alerts, notifications
        for section in ["config", "alerts", "notifications"]:
            response, data[section] = load_json_file(os_join(data["config_dir"], f"{section}.json"))
            result = result and response
        # objects
        response, objects = load_objects(os_join(abspath(data["config_dir"]), "objects.yml"))
        data["objects"] = objects
        result = result and response
        # filters
        data["filters"] = dict()
        data["compiled_re"] = dict()
        for matcher in [
            "alert_limits",
            "notifications",
            "flaps",
            "disaster",
            "abc",
            "namespace",
            "tags",
        ]:
            response, data["filters"][matcher] = load_csv_file(
                os_join(abspath(data["config_dir"]), f"match_{matcher}.csv")
            )
            if response:
                for item in [x["regex"] for x in data["filters"][matcher]]:
                    precompile_regex(item, data["compiled_re"])
            result = result and response
        for alert_re in [
            single_re.strip('"')
            for alert in data["alerts"]
            for single_re in data["alerts"][alert].get("apply_to") + data["alerts"][alert].get("dont_apply_to", [])
        ]:
            precompile_regex(alert_re, data["compiled_re"])
        if "default_locations" not in data["config"]:
            data["config"]["default_locations"] = LOCATIONS
        return result
    return False


# --------------------------------------------------------------------------------------------------
def precompile_regex(regex: str, regexes: Dict[str, Pattern]) -> None:
    """
    Прекомпилирует регулярное выражение и добавляет в словарь

    :param regex: регулярное выражение
    :type regex: str
    :param regexes: словарь прекомпилированных регулярных выражений
    :type regexes: Dict[str, Pattern]
    """
    if regex not in regexes:
        regexes[regex] = re_compile(regex)


# --------------------------------------------------------------------------------------------------
def check_or_create_dir(path: str) -> None:
    """
    Проверяет существует ли директория и создает ее

    :param path: Путь директории
    :type path: str
    """
    log: Logger = getLogger("yasm-client")
    if not isdir(path):
        try:
            mkdir(path)
        except OSError as error:
            if error.errno != EEXIST:
                log.info("%s существует", path)


# --------------------------------------------------------------------------------------------------
def str_replace(string: str, pairs: List) -> str:
    """
    Утилитарная функция замены в строке

    :param string: Исходная строка
    :type string: str
    :param pairs: Список кортежей (искомое, замена)
    :type pairs: List
    :return: Строка после замен
    :rtype: str
    """
    for pair in pairs:
        string = string.replace(*pair)
    return string


# --------------------------------------------------------------------------------------------------
def alerts_by_host(alerts: Dict) -> Dict[str, List]:
    """
    Возвращает словарь с разбиением полных названий алертов по хостам

    :param alerts: Словарь алертов
    :type alerts: Dict
    :return: Словарь слертов с разбиением по хостам
    :rtype: Dict[str, List]
    """
    result: Dict[str, List] = dict()
    for host, alert in [
        (alerts[alert]["juggler_check"]["host"], alert) for alert in alerts if alerts[alert].get("juggler_check", None)
    ]:
        if host not in result:
            result[host] = list()
        result[host].append(alert)
    return result


# --------------------------------------------------------------------------------------------------
def alerts_by_host_and_dc(alerts: Dict) -> Dict[str, Dict]:
    """
    Возвращает словарь с разбиением полных названий алертов по хостам и затем по дц

    :param alerts: Словарь алертов
    :type alerts: Dict
    :return: Словарь слертов с разбиением по хостам
    :rtype: Dict[str, Dict]
    """
    result: Dict[str, Dict] = dict()
    for host, alert in [
        (alerts[alert]["juggler_check"]["host"], alert) for alert in alerts if alerts[alert].get("juggler_check", None)
    ]:
        loc: str = alert_extract_geo(alerts[alert])
        if host not in result:
            result[host] = dict()
        if loc not in result[host]:
            result[host][loc] = list()
        result[host][loc].append(alert)
    return result


# --------------------------------------------------------------------------------------------------
def alert_extract_geo(alert: Dict) -> str:
    """
    Извлекает тэг geo из словаря алерта

    :param alert: словарь алерта
    :type alert: Dict
    :return: Значение гео, либо None
    :rtype: str
    """
    if "tags" not in alert:
        return "nodc"
    if "geo" not in alert["tags"]:
        return "nodc"
    return alert["tags"]["geo"][0]
