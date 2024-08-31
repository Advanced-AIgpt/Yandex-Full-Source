#!/usr/bin/env python
# -*- coding: utf-8 -*-

from collections import defaultdict

from constants import (
    OPEN_EVENTS,
    EVENT_MAP,
    SCREENS,
)

from utils.common import (
    get_dict_path,
)


class ScreensInfo(object):

    def __init__(self):
        self.screens_info = defaultdict(int)
        self.active_outer_screen = None

    # Метод сравнивает новый внешний экран с текущим, при различии инкрементирует соответствующий этому экрану счетчик
    # активности (кликов) в screen_info
    def update_active_outer_screen(self, active_screens):
        for active_screen in active_screens:
            if SCREENS.get(active_screen) is None:
                continue

            active_screen = SCREENS[active_screen].get('name') or active_screen
            if SCREENS[active_screen]['type'] == 'outer' and self.active_outer_screen != active_screen:
                self.screens_info[active_screen] += 1
                self.active_outer_screen = active_screen

    def add_event(self, record):
        event_name = record['event_name']
        screens = []
        if event_name in ['session_init', 'session_end']:
            self.active_outer_screen = None
        elif event_name in ['card_click', 'carousel_scroll', 'category_scroll']:
            screens.append(get_dict_path(record, ['event_value', 'place']))
        else:
            if event_name in OPEN_EVENTS:
                screens.append(get_dict_path(record, ['event_value', 'startup_place', 'from']))
            if event_name in EVENT_MAP:
                screens.append(EVENT_MAP[event_name])
        if screens:
            self.update_active_outer_screen(screens)

    def get_info(self):
        return dict(self.screens_info)
