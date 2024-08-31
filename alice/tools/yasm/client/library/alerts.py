# -*- coding: utf-8 -*-
"""
Библиотека функций доставки алёртов в Голован
"""
from asyncio import Semaphore
from asyncio import get_event_loop, gather, ensure_future
from json import dumps as j_dumps
from logging import Logger, getLogger
from os import listdir, unlink
from os.path import join as os_join, isfile, splitext, abspath, basename
from typing import Dict, List, Optional
from re import Pattern
from copy import deepcopy
from pprint import pformat
from aiohttp import ClientSession
from alice.tools.yasm.client.library.aiohttp_client import bound_send_http
from alice.tools.yasm.client.library.common import (
    YASM_API_HOST,
    load_json_file,
    check_or_create_dir,
    load_configs,
    gen_full_alert_name,
    alerts_by_host,
)
from alice.tools.yasm.client.library.compare import compare_lists, compare_alerts_left
from alice.tools.yasm.client.library.matching import (
    match_at_least_one,
    match_disaster,
    match_abc,
    match_flaps,
    match_alert_limits,
    match_tags,
    match_namespace,
    match_notifications,
)
from alice.tools.yasm.client.library.arguments import ARGS
from alice.tools.yasm.client.library.types import ReqStatus

BATCH_SIZE: int = 10


# === Сетевые взаимодействия с Голованом ===
# --------------------------------------------------------------------------------------------------
async def get_alerts_of_host(session: ClientSession, host: str, sem: Optional[Semaphore] = None) -> Dict:
    """
    Дозапрашивает алерты, если в первом запросе пришли не все

    :param session: Объект, клиентская сессия aiohttp
    :type session: ClientSession
    :param host: Имя хоста
    :type host: str
    :param sem: Объект семафор для ограничения параллельности выполнения
    :type sem: Semaphore
    :return: Содержимое ответа сервера
    :rtype: Dict
    """
    url: str = f"{YASM_API_HOST}/alerts/list?name_pattern={host}"
    url += f"&with_checks=true&limit={BATCH_SIZE}"
    result: Dict = dict()
    data: Dict = (await bound_send_http(session, "get", url, host, sem))["data"]["response"]
    if data["total"] > len(data["result"]):
        for offset in range(len(data["result"]), data["total"], BATCH_SIZE):
            response: ReqStatus = await bound_send_http(session, "get", url + f"&offset={offset}", host, sem)
            if "success" in response:
                data["result"].extend(response["data"]["response"]["result"])
    for alert in data.get("result", {}):
        result[alert["name"]] = alert
    return result


# --------------------------------------------------------------------------------------------------
def get_alerts(hosts: List, parallel_count: int) -> Dict:
    """
    Получить из Голована все алерты переданного списка хостов

    :param hosts: Список имен хостов, для которых следует получить алерты
    :type hosts: List
    :param parallel_count: Количество потоков HTTP клиента
    :type parallel_count: int
    :return: Список с алертами
    :rtype: List
    """
    log: Logger = getLogger("yasm-client")
    log.info("Запрашиваем алерты для %s хостов", len(hosts))

    async def runner():
        sem: Semaphore = Semaphore(parallel_count)
        tasks: List = list()
        async with ClientSession() as session:
            for cl_host in hosts:
                tasks.append(ensure_future(get_alerts_of_host(session, cl_host, sem)))
            responses = await gather(*tasks)
            return responses

    loop = get_event_loop()
    data: Dict = loop.run_until_complete(ensure_future(runner()))
    alerts: Dict = dict()
    for host in data:
        alerts.update(host)
    return alerts


# --------------------------------------------------------------------------------------------------
def delete_alerts(alerts: List, parallel_count: int) -> Dict:
    """
    Удаление списка алёртов

    :param alerts: Список алертов для удаления
    :type alerts: List
    :param parallel_count: Количество потоков HTTP клиента
    :type parallel_count: int
    :return: Ответ сервера
    :rtype: Dict
    """
    headers: Dict = {"Content-Type": "application/json", "Accept": "application/json"}

    async def runner():
        sem: Semaphore = Semaphore(parallel_count)
        tasks: List = list()
        async with ClientSession() as session:
            for alert in alerts:
                url: str = f"{YASM_API_HOST}/alerts/delete?name={alert}"
                tasks.append(ensure_future(bound_send_http(session, "post", url, alert, sem, headers=headers)))
            responses = await gather(*tasks)
            return responses

    loop = get_event_loop()
    data: Dict = loop.run_until_complete(ensure_future(runner()))
    return data


# --------------------------------------------------------------------------------------------------
def upsert_alerts(alerts_update: List, alerts_create: List, alert_data: Dict, parallel_count: int) -> Dict:
    """
    Создание и обновление алертов по спискам

    :param alerts_update: Список алертов для обновления
    :type alerts_update: List
    :param alerts_create: Список алертов для создания
    :type alerts_create: List
    :param alert_data: Данные алертов
    :type alert_data: Dict
    :param parallel_count: Количество потоков HTTP клиента
    :type parallel_count: int
    :return: Словарь JSON ответов сервера
    :rtype: Dict
    """
    headers: Dict = {"Content-Type": "application/json", "Accept": "application/json"}

    async def runner():
        sem: Semaphore = Semaphore(parallel_count)
        tasks: List = list()
        async with ClientSession() as session:
            for alert in alerts_update:
                url: str = f"{YASM_API_HOST}/alerts/update?name={alert}"
                tasks.append(
                    ensure_future(
                        bound_send_http(
                            session,
                            "post",
                            url,
                            alert,
                            sem,
                            headers=headers,
                            data=j_dumps(alert_data[alert]),
                        )
                    )
                )
            for alert in alerts_create:
                url: str = f"{YASM_API_HOST}/alerts/create"
                tasks.append(
                    ensure_future(
                        bound_send_http(
                            session,
                            "post",
                            url,
                            alert,
                            sem,
                            headers=headers,
                            data=j_dumps(alert_data[alert]),
                        )
                    )
                )
            responses = await gather(*tasks)
            return responses

    loop = get_event_loop()
    data: Dict = loop.run_until_complete(ensure_future(runner()))
    return data


# === Работа со списками действий ===


# -------------------------------------------------------------------------------------------------
def create_alert_actions(new_alerts: Dict, parallel_count: int, force_diff: bool = False) -> Dict:
    """
    Подготовка изменений алертов для Голована

    :param new_alerts: данные алертов
    :type new_alerts: Dict
    :param parallel_count: количество параллельных процессов
    :type parallel_count: int
    :param force_diff: выводить diff для алертов
    :type force_diff: bool
    :return Dict: подготовленные данные для отправки в Голован
    :rtype: Dict
    """
    log: Logger = getLogger("yasm-client")
    log.info("Вычисляем разницу с алертами Голована")
    alert_actions: Dict = dict()
    alert_actions["create"] = list()
    alert_actions["delete"] = list()
    alert_actions["update"] = list()
    new_alerts_by_host: Dict = alerts_by_host(new_alerts)
    old_alerts: Dict = get_alerts([*new_alerts_by_host], parallel_count)
    old_alerts_by_host: Dict = alerts_by_host(old_alerts)
    for host in new_alerts_by_host:
        if host in old_alerts_by_host:
            new_not_old, old_not_new, both = compare_lists(new_alerts_by_host[host], old_alerts_by_host[host])
            alert_actions["create"] += new_not_old
            alert_actions["delete"] += old_not_new
            for alert in both:
                if diff := compare_alerts_left(new_alerts[alert], old_alerts[alert]):
                    alert_actions["update"].append(alert)
                    if force_diff:
                        log.warning("%s\n%s", alert, pformat(diff, indent=2))
                    else:
                        log.debug("%s\n%s", alert, pformat(diff, indent=2))
        else:
            alert_actions["create"] += new_alerts_by_host[host]
    return alert_actions


# --------------------------------------------------------------------------------------------------
def print_summary(alert_actions: Dict[str, List]) -> None:
    """
    Вывод действий над алертами

    :param alert_actions: Словарь со списками алертов
    :type alert_actions: Dict[str, List]
    """
    log: Logger = getLogger("yasm-client")
    if "create" in alert_actions:
        total = len(alert_actions["create"])
        log.info("Алерты, которые надо создать [%s]:", total)
        for alert in sorted(alert_actions["create"]):
            log.info("  %s", alert)
    else:
        log.info("Нет алертов, которые надо создать")
    if "delete" in alert_actions:
        total = len(alert_actions["delete"])
        log.info("Алерты, которые надо удалить [%s]:", total)
        for alert in sorted(alert_actions["delete"]):
            log.info("  %s", alert)
    else:
        log.info("Нет алертов, которые надо удалить")
    if "update" in alert_actions:
        total = len(alert_actions["update"])
        log.info("Алерты, которые надо обновить [%s]", total)
        for alert in sorted(alert_actions["update"]):
            log.info("  %s", alert)
    else:
        log.info("Нет алертов, которые надо обновить")


# === Чтение и запись алертов ===
# --------------------------------------------------------------------------------------------------
def load_alerts(path: str) -> Dict:
    """
    Читает .json файлы в переданной директории и возвращает их в словаре

    :param path: Путь директории с файлами алертов
    :type path: str
    :return: Словарь с данными алертов
    :rtype: Dict
    """
    alerts: Dict = dict()
    for alert_filename in listdir(path):
        alert_filename_full: str = os_join(path, alert_filename)
        if isfile(alert_filename_full):
            key, _ = splitext(alert_filename)
            alerts[key] = load_json_file(alert_filename_full)
    return alerts


# --------------------------------------------------------------------------------------------------
def remove_obsolete(path: str, alert_list: List) -> None:
    """
    Удаляет файлы алертов, не вошедших в сгенерированный список

    :param path: Путь для сохранения алертов
    :type path: str
    :param alert_list: Список имен алертов
    :type alert_list: List
    """
    log: Logger = getLogger("yasm-client")
    counter: int = 0
    for alert_file in listdir(abspath(path)):
        alert_path: str = os_join(abspath(path), alert_file)
        alert_name: str = splitext(basename(alert_file))[0]
        if alert_name not in alert_list and isfile(alert_path):
            try:
                unlink(alert_path)
            except IOError as error:
                log.error("Невозможно удалить файл %s: %s", alert_path, error)
            else:
                counter += 1
    if counter:
        log.info("Удалено %s JSON файлов алертов", counter)


# --------------------------------------------------------------------------------------------------
def write_alerts(path: str, alerts: Dict) -> None:
    """
    Записывает алерты в переданную директорию в виде раздельных JSON файлов

    :param path: Путь для сохранения алертов
    :type path: str
    :param alerts: Словарь с алертами
    :type alerts: Dict
    """
    log: Logger = getLogger("yasm-client")
    check_or_create_dir(path)
    remove_obsolete(path, [*alerts])
    counter: int = 0
    for alert in alerts:
        alert_path: str = os_join(abspath(path), alert + ".json")
        try:
            with open(alert_path, "w") as f_alert:
                f_alert.write(j_dumps(alerts[alert], indent=4, sort_keys=True))
                counter += 1
        except IOError as error:
            log.error("Невозможно писать в файл %s: %s", alert_path, error)
    if counter:
        log.info("Записано %s JSON файлов алертов", counter)


# === Функции генерации алертов ===
# --------------------------------------------------------------------------------------------------
def get_source(object_name: str, objects_data: Dict, source: Optional[str] = None) -> Dict:
    """
    Опциональная подмена источника сигнала алерта

    :param object_name: Имя объекта алертов
    :type object_name: str
    :param objects_data: Список всех объектов алертов
    :type objects_data: List
    :param source: Название объекта для подмены
    :type source: str
    :return: Словарь, описывающий объект алерта
    :rtype: Dict
    """
    result: Dict = objects_data[object_name]
    if source:
        for src_obj in objects_data:
            if src_obj == source:
                result = objects_data[src_obj]
                break
    return result


# --------------------------------------------------------------------------------------------------
def validate_alert_value_modify(value_modify: Dict) -> bool:
    """
    Проверка валидности поля 'value_modify' алерта

    :param value_modify: Словарь с данными интервальной функции
    :type value_modify: Dict
    :return: Валидна или нет интервальная функция
    :rtype: bool
    """
    log: Logger = getLogger("yasm-client")
    result: bool = True
    if "window" not in value_modify:
        log.error('Отсутствует параметр "window" в секции "value_modify"')
        result = False
    elif "type" not in value_modify:
        log.error('Отсутствует параметр "type" в секции "value_modify"')
        result = False
    elif not isinstance(value_modify["window"], int):
        log.error(
            'Невереный тип параметра "window" %s в секции "value_modify"',
            type(value_modify["window"]),
        )
        result = False
    elif not isinstance(value_modify["type"], str):
        log.error('Невереный тип параметра "type" %s в секции "value_modify"', type(value_modify["type"]))
        result = False
    elif value_modify["window"] < 0 or value_modify["window"] > 3600:
        log.error('Некорректное значение "window"=%s в секции "value_modify"', value_modify["window"])
        result = False
    elif value_modify["type"] not in ("aver", "max", "min", "summ"):
        log.error('Некорректное значение "type"=%s в секции "value_modify"', value_modify["type"])
        result = False
    return result


# --------------------------------------------------------------------------------------------------
def validate_alert_trend(trend_type: str, interval: int) -> bool:
    """
    Проверка трендового алерта

    :param trend_type: Типа трендового алерта
    :type trend_type: str
    :param interval: Интервал трендового алерта
    :type interval: int
    :return: Валиден или нет трендовый алерт
    :rtype: bool
    """
    log: Logger = getLogger("yasm-client")
    result: bool = True
    if trend_type not in ["up", "down"]:
        log.error('Типа трендового алерта должен быть либо "up", либо "down". Тип: %s', trend_type)
        result = False
    if interval < 25 or interval > 1800:
        log.error("Интервал должен быть между 25 и 1800 секунд. Интервал: %s", interval)
        result = False
    return result


# --------------------------------------------------------------------------------------------------
def fill_alert_limits(
    alert_name: str,
    alert: Dict,
    alert_limits: List[Dict],
    compiled_re: Dict[str, Pattern],
    trend: Optional[str] = None,
    interval: Optional[int] = None,
    perc: bool = False,
) -> None:
    """
    Формирует лимиты алерта

    :param alert_name: Название алерта
    :type alert_name: str
    :param alert: Словарь заполняемого алерта
    :type alert: Dict
    :param alert_limits: Список лимитов
    :type alert_limits: List[Dict]
    :param compiled_re: Словарь скомилированных регулярных выражений
    :type compiled_re: Dict[str, Pattern]
    :param trend: Тип тренда
    :type trend: Optional[str]
    :param interval: Интервал тренда
    :type interval: int
    :param perc: Флаг, являются ли пороги тренда процентными
    :type perc: bool
    """
    warn_limit, crit_limit = match_alert_limits(alert_name, alert_limits, compiled_re)
    if trend and interval:
        if validate_alert_trend(trend, interval):
            alert["trend"] = trend
            alert["interval"] = interval
        if perc:
            if warn_limit:
                alert["warn_perc"] = max([int(i) for i in warn_limit if i is not None])
            if crit_limit:
                alert["crit_perc"] = max([int(i) for i in crit_limit if i is not None])
            return
    alert["warn"] = warn_limit
    alert["crit"] = crit_limit


# --------------------------------------------------------------------------------------------------
def make_alert(obj: Dict, alert: str, data: Dict, location: Optional[str] = None) -> Dict:
    """
    Создание JSON файла алерта

    :param obj: Словарь, описывающий обхект алерта
    :type obj: Dict
    :param alert: Название алерта
    :type alert: str
    :param data: Данные для формирования алерта
    :type data: Dict
    :param location: Опциональный параметр - локация
    :type location: str
    :return: Словарь, описывающий алерта в формате Голована
    :rtype: Dict
    """
    alert_name: str = data["alerts"][alert].get("name", alert)
    alert_name_obj: str = gen_full_alert_name(obj, alert_name)
    alert_name_full = alert_name_obj if not location else f"{alert_name_obj}_{location.lower()}"
    alert_name_short = alert_name if not location else f"{alert_name}_{location.lower()}"
    new_alert: Dict = deepcopy(data["config"]["alert_tmpl"])
    new_alert["name"] = f"{alert_name_full}"
    source: Dict = get_source(obj["name"], data["objects"], data["alerts"][alert].get("source"))
    new_alert["signal"] = data["alerts"][alert]["signal"]
    new_alert["mgroups"].append(source["hosts"])
    new_alert["disaster"] = match_disaster(alert_name_full, data["filters"]["disaster"], data["compiled_re"])
    new_alert["tags"]["itype"] = list()
    new_alert["tags"]["itype"].append(source["itype"])
    new_alert["tags"]["ctype"] = list()
    if isinstance(source["ctype"], list):
        new_alert["tags"]["ctype"] += source["ctype"]
    else:
        new_alert["tags"]["ctype"].append(source["ctype"])
    if location:
        new_alert["tags"]["geo"] = list()
        new_alert["tags"]["geo"].append(location.lower())
    if "prj" in source:
        new_alert["tags"]["prj"] = list()
        new_alert["tags"]["prj"].append(source["prj"])
    if value_modify := data["alerts"][alert].get("value_modify", None):
        if validate_alert_value_modify(value_modify):
            new_alert["value_modify"] = value_modify
    fill_alert_limits(
        alert_name_full,
        new_alert,
        data["filters"]["alert_limits"],
        data["compiled_re"],
        data["alerts"][alert].get("trend"),
        data["alerts"][alert].get("interval"),
        data["alerts"][alert].get("perc"),
    )
    new_alert["juggler_check"]["host"] = obj["name"]
    new_alert["juggler_check"]["service"] = alert_name_short
    new_alert["juggler_check"]["namespace"] = match_namespace(
        alert_name_full, data["filters"]["namespace"], data["compiled_re"]
    )
    new_alert["juggler_check"]["notifications"] = match_notifications(
        alert_name_full,
        data["notifications"],
        data["filters"]["notifications"],
        data["compiled_re"],
        data["alerts"][alert].get("skip_default", False),
    )
    new_alert["juggler_check"]["flaps"] = match_flaps(alert_name_full, data["filters"]["flaps"], data["compiled_re"])
    if all(
        [
            new_alert["juggler_check"]["flaps"]["critical"] == 0,
            new_alert["juggler_check"]["flaps"]["stable"] == 0,
            new_alert["juggler_check"]["flaps"]["boost"] == 0,
        ]
    ):
        new_alert["juggler_check"]["flaps"] = None
    if "nodata_mode" in source:
        new_alert["juggler_check"]["aggregator_kwargs"]["nodata_mode"] = source["nodata_mode"]
    if owner := match_abc(alert_name_full, data["filters"]["abc"], data["compiled_re"]):
        new_alert["abc"] = owner
    if new_tags := match_tags(alert_name_full, data["filters"]["tags"], data["compiled_re"], new_alert["disaster"]):
        new_alert["juggler_check"]["tags"] = new_tags
    return new_alert


# -------------------------------------------------------------------------------------------------
def make_alerts(data: Dict) -> None:
    """
    Генерация списка алёртов

    :param data: Данные для генерации
    :type data: Dict
    """
    log: Logger = getLogger("yasm-client")
    if "made_alerts" not in data:
        data["made_alerts"] = dict()
    if "alert_prefixes" not in data:
        data["alert_prefixes"] = dict()
    for alert in data["alerts"]:
        for obj in data["objects"]:
            if match_at_least_one(
                obj, data["alerts"][alert].get("apply_to"), data["compiled_re"]
            ) and not match_at_least_one(obj, data["alerts"][alert].get("dont_apply_to", None), data["compiled_re"]):
                loc_list: List[Optional[str]] = [None]
                if not ("no_locations" in data["objects"][obj] or "no_locations" in data["alerts"][alert]):
                    loc_list = data["objects"][obj].get("locations", data["config"]["default_locations"])
                for loc in loc_list:
                    new_alert: Dict = make_alert(data["objects"][obj], alert, data, loc)
                    data["made_alerts"][new_alert["name"]] = new_alert
                    if "name_prefix" in data["alerts"][alert]:
                        data["alert_prefixes"][new_alert["name"]] = data["alerts"][alert]["name_prefix"]
                    else:
                        log.error("Для алерта %s не указан name_prefix", alert)


# === Основные функции ===
# --------------------------------------------------------------------------------------------------
def alert_main(data: Dict) -> None:
    """
    Генерация и применение алёртов

    :param data: Данные для генерации
    :type data: Dict
    """
    log: Logger = getLogger("yasm-client")
    if data.get("alerts_dir"):
        log.info("Загружаем JSON файлы алертов из %s", abspath(data["alerts_dir"]))
        data["made_alerts"] = load_alerts(data["alerts_dir"])
        log.info("Загружено %s алертов", len(data["made_alerts"]))
    else:
        if not load_configs(data):
            log.critical("Не удалось загрузить конфиги из %s", data.get("config_dir", ""))
            return
        make_alerts(data)
        log.info("Сгенерированно %s алертов", len(data["made_alerts"]))
        if data["save_alert_dir"]:
            log.info("Сохраняем JSON файлы алертов в %s", abspath(data["save_alert_dir"]))
            write_alerts(data["save_alert_dir"], data["made_alerts"])
    alert_actions = create_alert_actions(data["made_alerts"], data["parallel_count"], data["f_diff"])
    if not (alert_actions.get("create") or alert_actions.get("delete") or alert_actions.get("update")):
        log.info("Нет разницы с алертами Голована")
        return
    print_summary(alert_actions)
    if not data["f_dry_run"]:
        upsert_alerts(
            alert_actions["update"],
            alert_actions["create"],
            data["made_alerts"],
            data["parallel_count"],
        )
        log.info("ВАЖНО")
        log.info("Не забудь замерджить эти изменения в транк, иначе они перебьются следующим коммитом.")
        log.info(
            "А лучше, не делай так больше, а просто замерджи конфиг в транк"
            + " и дай применить изменения автоматическому флоу."
        )
        if not data["f_do_not_delete"]:
            delete_alerts(alert_actions["delete"], data.get("parallel_count", 1))
        else:
            log.info("Удаление не производим из-за аргумента %s", ARGS["f_do_not_delete"])
    else:
        log.info("Изменения не применяем из-за аргумента %s", ARGS["f_dry_run"])


# -------------------------------------------------------------------------------------------------
def alert_wipe(data: Dict) -> None:
    """
    Удаление алертов в Голован

    :param data: Данные алертов
    :type data: Dict
    """
    log: Logger = getLogger("yasm-client")
    log.info("Удаляем %s", data["target"])
    alert_actions: Dict = dict()
    alert_actions["delete"] = list()
    old_alerts: Dict = get_alerts([data["target"]], data.get("parallel_count", 1))
    old_alerts_by_host: Dict = alerts_by_host(old_alerts)
    for host in old_alerts_by_host:
        alert_actions["delete"] += old_alerts_by_host[host]
    print_summary(alert_actions)
    if not (data["f_do_not_delete"] or data["f_dry_run"]):
        delete_alerts(alert_actions["delete"], data["parallel_count"])
    else:
        log.info("Удаление не производим из-за аргумента %s", ARGS["f_do_not_delete"])
