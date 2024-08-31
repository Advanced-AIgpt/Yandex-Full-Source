#!/usr/bin/env python
# encoding: utf-8
import os

import yt.wrapper


def make_simple_client():
    token = os.environ.get('YT_TOKEN')  # Почему-то, сам yt.wrapper переменную окружения не читает.
    #  Если она не установлена, то token=None и аутентификация отдаётся на откуп yt-клиенту
    return yt.wrapper.YtClient(proxy='hahn',
                               token=token,
                               config={"tabular_data_format": "tsv"})
