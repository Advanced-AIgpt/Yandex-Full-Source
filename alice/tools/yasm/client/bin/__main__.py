#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Клиент Голована. Генерирует, применяет и организует в дашборды алёрты
"""
from sys import argv
from logging import Logger, basicConfig, getLogger, INFO, DEBUG, WARNING
from typing import Dict
from alice.tools.yasm.client.library import (
    alert_main,
    alert_wipe,
    dashboard_main,
    get_arguments,
    verify_main,
)


# --------------------------------------------------------------------------------------------------
def init_logging(options: Dict) -> None:
    """
    Начальная настройка модуля логирования

    :param options: Словарь с аргументами вызова
    :type options: Dict
    """
    l_level: int = INFO
    if options["f_verbose"]:
        l_level = DEBUG
    elif options["f_warning"]:
        l_level = WARNING
    l_format: str = "%(asctime)s %(levelname)12s: %(message)s"
    if options.get("f_log"):
        basicConfig(level=l_level, format=l_format, filename=options["f_log"], filemode="a")
    else:
        basicConfig(level=l_level, format=l_format)
    urllib_logger: Logger = getLogger("urllib3.connectionpool")
    urllib_logger.setLevel(INFO)


# -------------------------------------------------------------------------------------------------
def main() -> None:
    """
    Основная функция
    """
    args: Dict = get_arguments(argv[1:])
    init_logging(args)
    if args.get("f_wipe", None):
        alert_wipe(args)
        return
    if args.get("f_verify", None):
        verify_main(args)
    if args.get("f_alerts", None):
        alert_main(args)
    if args.get("f_dashboard", None):
        dashboard_main(args)


if __name__ == "__main__":
    main()
