# -*- coding: utf-8 -*-
"""
Обвязка асинхронного клиента aiohttp для запросов в Голован
"""
from typing import Optional
from logging import Logger, getLogger
from asyncio import Semaphore
from aiohttp import ClientSession, ClientConnectionError
from tenacity import (
    retry,
    wait_exponential,
    stop_after_attempt,
    retry_if_exception_type,
    RetryError,
)
from alice.tools.yasm.client.library.types import ReqStatus


# --------------------------------------------------------------------------------------------------
@retry(
    retry=(retry_if_exception_type(ClientConnectionError)),
    wait=wait_exponential(multiplier=1, min=1, max=120),
    stop=stop_after_attempt(10),
)
async def send_http(
    session: ClientSession, method: str, url: str, obj_name: str, *args, **kwargs
) -> Optional[ReqStatus]:
    """
    Делает HTTP запрос и реализует логику перезапросов

    :param session: Объект, клиентская сессия aiohttp
    :type session: ClientSession
    :param method: HTTP метод
    :type method: str
    :param url: URL запроса
    :type url: str
    :param obj_name: Имя объекта запроса
    :type obj_name: str
    :return: Содержимое ответа сервера
    :rtype: ReqStatus
    """
    log: Logger = getLogger("yasm-client")
    result: ReqStatus = ReqStatus(success=False, obj_name=obj_name, url=url, code=0, error="", data={})
    if method not in ["get", "post"]:
        log.error("%s неверный метод %s", obj_name, method)
        return result
    try:
        async with getattr(session, method)(url, timeout=15, *args, **kwargs) as response:
            result["data"] = await response.json()
            result["code"] = response.status
    except Exception as ex:
        log.error("%s %s", obj_name, repr(ex))
        return result
    if result["code"] < 300:
        log.debug("%s %s выполнено", obj_name, result["code"])
        result["success"] = True
        return result
    log.error("%s %s ошибка. %s", obj_name, result["code"], result["data"].get("error"))
    return result


# --------------------------------------------------------------------------------------------------
async def bound_send_http(
    session: ClientSession, method: str, url: str, obj_name: str, sem: Optional[Semaphore], **kwargs
) -> ReqStatus:
    """
    Обёртка над send_http с возможностью использовать семафоры

    :param session: Объект, клиентская сессия aiohttp
    :type session: ClientSession
    :param method: HTTP метод
    :type method: str
    :param rl: URL запроса
    :type url: str
    :param obj_name: Имя объекта запроса
    :type obj_name: str
    :param sem: Объект семафор для ограничения параллельности выполнения
    :type sem: Semaphore
    :return: Содержимое ответа сервера
    :rtype: Dict
    """
    log: Logger = getLogger("yasm-client")
    result: ReqStatus = ReqStatus()
    try:
        if sem:
            async with sem:
                result = await send_http(session, method, url, obj_name, **kwargs)
        else:
            result = await send_http(session, method, url, obj_name, **kwargs)
    except RetryError:
        log.critical("%s %s завершено неуспешно. Попытки перезапросов исчерпаны", obj_name, url)
    return result
