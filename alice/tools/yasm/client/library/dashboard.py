# -*- coding: utf-8 -*-
"""
Создание и обновление дашбордов в Головане
"""
from asyncio import get_event_loop, ensure_future
from json import dumps as j_dumps
from os.path import join as os_join, abspath, split as os_split
from logging import Logger, getLogger
from typing import Dict, List, Tuple, Optional
from uuid import uuid4
from copy import deepcopy
from aiohttp import ClientSession
from alice.tools.yasm.client.library.aiohttp_client import send_http
from alice.tools.yasm.client.library.common import (
    YASM_API_HOST,
    REPLACE_PAIRS,
    load_json_file,
    load_configs,
    str_replace,
    gen_alert_label,
    alerts_by_host_and_dc,
)
from alice.tools.yasm.client.library.types import YasmWidget
from alice.tools.yasm.client.library.compare import (
    comparator_alert,
    comparator_itype,
    comparator_dc,
)
from alice.tools.yasm.client.library.alerts import make_alerts
from alice.tools.yasm.client.library.charts import render_chart
from alice.tools.yasm.client.library.arguments import ARGS

HEADER_WIDTH: int = 1


# --------------------------------------------------------------------------------------------------
def get_panel(key: str, user: str) -> Dict:
    """
    Запросить JSON панели из Голована

    :param key: Ключ панели
    :type key: str
    :param user: Владелец панели
    :type user: str
    :return: Словарь с данными панели
    :rtype: Dict
    """
    log: Logger = getLogger("yasm-client")
    if not key:
        log.error("Не указан ключ панели")
        return {}
    if not user:
        log.error("Не указан владелец панели")
        return {}
    url = f"{YASM_API_HOST}/get?key={user}.{key}"

    async def runner():
        async with ClientSession() as session:
            resp: Dict = await send_http(session, "get", url, key)
        result = resp["data"].get("result", {})
        result.pop("updated", None)
        result.pop("_id", None)
        return result

    loop = get_event_loop()
    return loop.run_until_complete(ensure_future(runner()))


# --------------------------------------------------------------------------------------------------
def upsert_panel(post_data: Dict) -> Dict:
    """
    Загрузить JSON панели Голована

    :param Dict post_data: Словать с данными для загрузки
    :return: Словарь ответами сервера
    :rtype: Dict
    """
    url = f"{YASM_API_HOST}/upsert"

    async def runner():
        async with ClientSession() as session:
            result: Dict = await send_http(session, "post", url, post_data["keys"]["key"], data=j_dumps(post_data))
        return result

    loop = get_event_loop()
    return loop.run_until_complete(ensure_future(runner()))


# --------------------------------------------------------------------------------------------------
def get_widget_type(widget_desc: Dict) -> Optional[str]:
    """
    Определяет типа виджета по ключу в словаре

    :param Dict widget_desc: Описание виджета
    :return: Тип виджета
    :rtype: str
    """
    types = [("alert", "alert"), ("check", "check"), ("chart", "signals"), ("chart", "gen_chart")]
    for w_type in types:
        widget_type, widget_key = w_type
        if widget_key in widget_desc:
            return widget_type
    return None


# --------------------------------------------------------------------------------------------------
def get_widget(widget_desc: Dict, row: int, col: int, size: Tuple[int, int] = (1, 1)) -> YasmWidget:
    """
    Генерирует виджет Голована

    :param Dict widget_desc: Описание виджета
    :param int row: Номер строки
    :param int col: Номер столбца
    :param Tuple[int, int] size: Размер виджета (высота, ширина)
    :return: Словарь с описанием виджета
    :rtype: Dict
    """
    log: Logger = getLogger("yasm-client")
    width, height = size
    if "size" in widget_desc:
        width, height = tuple(widget_desc["size"])
    widget: YasmWidget = YasmWidget(
        id=str(uuid4()),
        width=width,
        height=height,
        col=col,
        row=row,
        type="",
        title="",
        links="",
        text="",
    )
    widget_type = get_widget_type(widget_desc)
    if widget_type in ["alert", "check", "chart"]:
        widget["type"] = widget_type
        widget["title"] = widget_desc["label"]
        if widget_type == "alert":
            widget["name"] = widget_desc["alert"]
        elif widget_type == "chart":
            if "gen_chart" not in widget_desc:
                widget["signals"] = render_chart(widget_desc["gen_chart"])
            else:
                widget["signals"] = widget_desc["signals"]
            if "stacked" in widget_desc:
                widget["stacked"] = widget_desc["stacked"]
            if "normalize" in widget_desc:
                widget["normalize"] = widget_desc["normalize"]
        else:
            widget["host"] = widget_desc["check"]["host"]
            widget["service"] = widget_desc["check"]["service"]
        if "links" in widget_desc:
            widget["links"] = widget_desc["links"]
    else:
        widget["type"] = "text"
        widget["text"] = widget_desc["label"]
    log.debug("Виджет: %s", widget_desc["label"])
    return widget


# --------------------------------------------------------------------------------------------------
def get_widget_size(widget_desc: Dict) -> Tuple:
    """
    Возвращает размер виджета в виде кортежа

    :param Dict widget_desc: Описание виджета
    :return: Кортеж (ширина, высота)
    :rtype: Tuple[int, int]
    """
    if "size" in widget_desc:
        return tuple(widget_desc["size"])
    return 1, 1


# --------------------------------------------------------------------------------------------------
def get_component_body_size(dashboard_conf: Dict, component: str) -> Tuple[int, int]:
    """
    Возвращает размер тела блока в виде кортежа

    :param Dict dashboard_conf: Данные панели
    :param str component: Имя блока
    :return: Кортеж (ширина, высота)
    :rtype: Tuple[int, int]
    """
    component_data: Dict = dashboard_conf["struct"].get(component)
    if "rows" not in component_data:
        return 0, 0
    row_widths: List = list()
    row_heights: List = list()
    for line in component_data["rows"]:
        row_width: int = 0
        row_max_height: int = 0
        for widget in line:
            w_width, w_height = get_widget_size(dashboard_conf["objects"][widget])
            row_width += w_width
            row_max_height = max(w_height, row_max_height)
        row_widths.append(row_width)
        row_heights.append(row_max_height)
    return max(row_widths), sum(row_heights)


# --------------------------------------------------------------------------------------------------
def get_component_label_size(component: Dict) -> Tuple[int, int]:
    """
    Возвращает размер заголовка блока в виде кортежа

    :param Dict component: Словарь, описывающий блок
    :return: Кортеж (ширина, высота)
    :rtype: Tuple[int, int]
    """
    size: Tuple[int, int] = (0, 0)
    if "no_label" not in component:
        size = (component.get("width", HEADER_WIDTH), component.get("height", 1))
    return size


# --------------------------------------------------------------------------------------------------
def yasm_charts(dashboard_conf: Dict) -> List[Dict]:
    """
    Секция charts словаря, описывающего дашборд Голована

    :param Dict dashboard_conf: Конфиг дашборда
    :return: Словарь в формате Голована
    :rtype: Dict
    """
    log: Logger = getLogger("yasm-client")
    cursor: Dict = dict()
    cursor["row"], cursor["col"] = (1, 1)
    result: List = list()
    for component in dashboard_conf["struct"]:
        label: Dict = dict()
        body: Dict = dict()
        block: Dict = dict()
        log.debug("Координаты курсора: [%s, %s]", cursor["row"], cursor["col"])
        log.info("Компонент: %s", component)
        label["width"], label["height"] = get_component_label_size(dashboard_conf["struct"][component])
        log.debug("Размер лэйбла компонента: %s x %s", label["width"], label["height"])
        body["width"], body["height"] = get_component_body_size(dashboard_conf, component)
        log.debug("Размер тела компонента: %s x %s", body["width"], body["height"])
        block["width"] = body["width"] + label["width"]
        block["height"] = max(body["height"], label["height"])
        block["dimensions"] = {
            "start": {"row": cursor["row"], "col": cursor["col"]},
            "end": {
                "row": cursor["row"] + block["height"] - 1,
                "col": cursor["col"] + block["width"] - 1,
            },
        }
        if block["width"] == 0 or block["height"] == 0:
            continue
        if (label["width"], label["height"]) != (0, 0):
            result.append(
                get_widget(
                    {"label": dashboard_conf["struct"][component].get("label", component)},
                    block["dimensions"]["start"]["row"],
                    block["dimensions"]["start"]["col"],
                    size=(label["width"], max(label["height"], body["height"])),
                )
            )
        if "rows" in dashboard_conf["struct"][component]:
            cursor["row"] = block["dimensions"]["start"]["row"]
            cursor["col"] = block["dimensions"]["start"]["col"] + label["width"]
            data_column: int = cursor["col"]
            for line in dashboard_conf["struct"][component]["rows"]:
                widget: Dict = dict()
                widget["height_max"] = 0
                for widget_index in line:
                    widget["data"] = dashboard_conf["objects"][widget_index]
                    widget["width"], widget["height"] = get_widget_size(widget["data"])
                    result.append(get_widget(widget["data"], cursor["row"], cursor["col"]))
                    widget["height_max"] = max(widget["height"], widget["height_max"])
                    cursor["col"] = cursor["col"] + widget["width"]
                cursor["row"] = cursor["row"] + widget["height_max"]
                cursor["col"] = data_column
        if "no_newline" in dashboard_conf["struct"][component]:
            cursor["row"] = block["dimensions"]["start"]["row"]
            cursor["col"] = block["dimensions"]["end"]["col"] + 1
        else:
            cursor["row"] = block["dimensions"]["end"]["row"] + 1
            cursor["col"] = 1
    return result


# -------------------------------------------------------------------------------------------------
def validate_conf(data: Dict) -> bool:
    """
    Валидация прочитанного конфига

    :param data: Данные дашборда
    :type data: Dict
    :return: bool валиден/не валиден
    :rtype: bool
    """
    log: Logger = getLogger("yasm-client")
    result: bool = True
    objects_list: List = [*data["objects"]]
    for component in data["struct"]:
        if "rows" in data["struct"][component]:
            for line in data["struct"][component]["rows"]:
                for item in line:
                    if item in objects_list:
                        objects_list.remove(item)
                    else:
                        log.critical('"%s" нет объекта "%s"', component, item)
                        result = False
    if objects_list:
        log.warning("Неиспользуемые объекты:")
        for item in objects_list:
            log.warning("  %s", item)
    return result


# --------------------------------------------------------------------------------------------------
def dashboard_format(data: Dict, formatter: Dict) -> None:
    """
    Форматирует дашборд

    :param formatter: Словарь с данными для форматирования
    :type formatter: Dict
    :param data: Данные
    :type data: Dict
    """
    log: Logger = getLogger("yasm-client")
    log.info("Дополнительное форматирование панели")
    formatted_dashboard = {"objects": data["objects"], "struct": dict()}
    formatter_list = formatter.get("format", None)
    empty_id_counter = 0
    if formatter_list is None:
        log.error('Нет "format" в файле форматирования панели')
        return
    for row in formatter_list:
        last_cell = ""
        for cell in row:
            if isinstance(cell, list) and len(cell) > 1:
                cell_name = f"e{empty_id_counter}"
                formatted_dashboard["struct"][cell_name] = dict(height=cell[0], width=cell[1])
                if len(cell) == 3 and cell[2] is True:
                    formatted_dashboard["struct"][cell_name]["no_newline"] = True
                empty_id_counter += 1
                last_cell = cell_name
                continue

            dash_cell = data["struct"].get(cell, None)
            if dash_cell is None:
                log.error("Объект определен в файле форматирования, но отсутствует в данных: %s", cell)
                continue
            formatted_dashboard["struct"][cell] = dash_cell
            formatted_dashboard["struct"][cell]["no_newline"] = True
            last_cell = cell
        formatted_dashboard["struct"][last_cell]["no_newline"] = False
    data = deepcopy(formatted_dashboard)


# -------------------------------------------------------------------------------------------------
def make_dashboard(objects: Dict, made_alerts: Dict, alert_prefixes: Dict) -> Dict:
    """
    Генерация словаря с описанием дашборда

    :param objects: словарь с днными объектов
    :type objects: Dict
    :param made_alerts: словарь со сгенерированными алертами
    :type made_alerts: Dict
    :param alert_prefixes: Префиксы алертов
    :type alert_prefixes: Dict
    :return: Словарь дашборда
    :type: Dict
    """

    def put_alert_in_list(
        alert_dict: Dict,
        alert_prefix: str,
        lst: List,
        location=None,
    ):
        dashboard_alert: Dict = gen_alert_label(alert_dict, alert_prefix, location)
        dashboard_object: str = str_replace(dashboard_alert["alert"], REPLACE_PAIRS)
        if dashboard_object not in made_dashboard["objects"]:
            made_dashboard["objects"][dashboard_object] = dashboard_alert
        lst.append(dashboard_object)

    made_dashboard: Dict = dict()
    made_dashboard["objects"] = dict()
    made_dashboard["struct"] = dict()
    alerts_sorted: Dict = alerts_by_host_and_dc(made_alerts)
    for obj in {k: v for k, v in sorted(objects.items(), key=comparator_itype)}:
        made_dashboard["struct"][objects[obj]["desc"]] = dict()
        made_dashboard["struct"][objects[obj]["desc"]]["rows"] = list()
        for loc in sorted(alerts_sorted[obj], key=comparator_dc):
            row: List = list()
            for alert in sorted(alerts_sorted[obj][loc], key=comparator_alert):
                put_alert_in_list(
                    made_alerts[alert],
                    alert_prefixes.get(alert, alert),
                    row,
                    loc,
                )
            made_dashboard["struct"][objects[obj]["desc"]]["rows"].append(row)
    return made_dashboard


# -------------------------------------------------------------------------------------------------
def write_dashboard(dashboard_data: Dict, path: str) -> None:
    """
    Запись дашборда в JSON файл

    :param dashboard_data: Данные панели
    :type dashboard_data: Dict
    :param path: Пусть для сохранения
    :type path: str
    """
    log: Logger = getLogger("yasm-client")
    try:
        with open(path, "w") as f_board:
            f_board.write(j_dumps(dashboard_data, indent=4, sort_keys=False))
    except IOError as error:
        log.error("Не могу записать файл %s: %s", abspath(path), error)
    else:
        log.info("Панель сохранена в %s", abspath(path))


# -------------------------------------------------------------------------------------------------
def gen_post_headers(args: Dict, made_dashboard: Dict) -> Tuple[bool, Dict]:
    """
    Хедеры для upsert запроса в Голован

    :param args: Аргументы командной строки, касающиеся панелей
    :type args: Dict
    :param made_dashboard: Словарь, описывающий панель
    :type made_dashboard: Dict
    :return: Словарь с хедерами
    :rtype: Dict
    """
    log: Logger = getLogger("yasm-client")
    result: Dict = dict()
    result["keys"] = dict()
    result["values"] = dict()
    for opt, name in [("key", "ключ"), ("user", "владелец")]:
        if opt not in made_dashboard and args[f"yasm_{opt}"] is None:
            log.critical("Не задан %s панели", name)
            return False, {}
        else:
            result["keys"][opt] = args[f"yasm_{opt}"] if args[f"yasm_{opt}"] else made_dashboard[opt]
    get_response: Dict = get_panel(result["keys"]["key"], result["keys"]["user"])
    for param in ["title", "editors"]:
        if param in get_response:
            result["values"][param] = get_response[param]
        if param in made_dashboard:
            result["values"][param] = made_dashboard[param]
        if args[f"yasm_{param}"]:
            result["values"][param] = args[f"yasm_{param}"]
    return True, result


# -------------------------------------------------------------------------------------------------
def dashboard_apply(data: Dict) -> None:
    """
    Внесение данных дашборда в Голован

    :param data: Данные панели
    :type data: Dict
    """
    log: Logger = getLogger("yasm-client")
    result, post_data = gen_post_headers(
        dict({k: v for k, v in data.items() if k.startswith("yasm_")}), data["made_dashboard"]
    )
    if not result:
        return
    post_data["values"]["charts"] = yasm_charts(data["made_dashboard"])
    resp_post: Dict = upsert_panel(post_data)
    if "success" in resp_post:
        if resp_post["success"]:
            log.info('Панель с ключем "%s" обновлена', resp_post["obj_name"])
            log.debug('  название панели "%s"', resp_post["data"]["result"]["title"])
            log.debug('  id панели "%s"', resp_post["data"]["result"]["_id"])
            log.debug('  вернул ключ панели "%s"', resp_post["data"]["result"]["key"])
            log.debug('  автор панели "%s"', resp_post["data"]["result"]["user"])
            if resp_post["data"]["result"]["editors"]:
                log.debug('  редакторы панели "%s"', ",".join(resp_post["data"]["result"]["editors"]))
            if "key" not in data["made_dashboard"]:
                data["made_dashboard"]["key"] = resp_post["data"]["result"]["_id"]
                data["made_dashboard"]["user"] = resp_post["data"]["result"]["user"]
        else:
            log.error("Ошибка изменения панели: %s", resp_post["error"])


# -------------------------------------------------------------------------------------------------
def dashboard_load_json(path: str) -> Dict:
    """
    Загрузка дерева JSON'ов панели

    :param path: Путь к файлу с данными дашборда
    :type path: str
    :return: Словарь с данными дашборда
    :rtype: Dict
    """
    made_dashboard: Dict
    _, made_dashboard = load_json_file(path)
    includes: List = made_dashboard.get("includes", None)
    if includes:
        for inc_path in includes:
            response, result = load_json_file(os_join(os_split(abspath(path))[0], inc_path))
            if response:
                made_dashboard["objects"].update(result.get("objects"))
    return made_dashboard


# -------------------------------------------------------------------------------------------------
def dashboard_main(data: Dict) -> None:
    """
    Генерация и применение панели

    :param data: Данные панели
    :type data: Dict
    """
    log: Logger = getLogger("yasm-client")
    if data["dashboard_json"]:
        data["made_dashboard"] = dashboard_load_json(data["dashboard_json"])
    else:
        load_configs(data)
        if "made_alerts" not in data:
            make_alerts(data)
        data["made_dashboard"] = make_dashboard(data["objects"], data["made_alerts"], data["alert_prefixes"])
    if data["format_json"]:
        _, data["formatter"] = load_json_file(data["format_json"])
        dashboard_format(data["made_dashboard"], data["formatter"])
    if not validate_conf(data["made_dashboard"]):
        log.critical("Конфиг дашборда не валиден")
    else:
        if not data["f_dry_run"]:
            dashboard_apply(data)
        else:
            log.info("Дашборд не применяем из-за аргумента %s", ARGS["f_dry_run"])
        if data["save_dashboard_json"]:
            write_dashboard(data["made_dashboard"], data["save_dashboard_json"])
