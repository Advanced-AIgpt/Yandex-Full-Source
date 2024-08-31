#!/usr/bin/env python
# -*- coding: utf-8 -*-

from constants import (
    SCREENS,
    UNDEFINED,
)

from utils.common import (
    get_dict_path,
)

from dayuse_utils import (
    increment_path,
    merge_info,
)


class CarouselsInfo(object):

    def __init__(self):
        self.info = {}
        self.midterm_info = {}
        self.active_outer_screen = None
        self.is_new_screen = True

    def update_active_outer_screen(self, screen):
        if get_dict_path(SCREENS, [screen, 'type']) == 'outer' and screen != self.active_outer_screen:
            self.active_outer_screen = screen
            self.info = merge_info(self.info, self.midterm_info)
            self.midterm_info = {}

    def add_event(self, record):
        event_name = record['event_name']

        if event_name == 'screen_changed':
            self.update_active_outer_screen(record['event_value'].get('to'))

        if event_name in ['card_click', 'card_show', 'carousel_show']:
            screen = record['event_value'].get('place', UNDEFINED)
            self.update_active_outer_screen(screen)

            carousel_name = record['event_value'].get('carousel_name', UNDEFINED)
            carousel_position = record['event_value'].get('y')

            if event_name == 'card_click':
                self.info = increment_path(
                    self.info,
                    [
                        screen,
                        carousel_name,
                        carousel_position,
                        None,
                        'click'
                    ],
                    1
                )
            self.midterm_info = increment_path(
                self.midterm_info,
                [
                    screen,
                    carousel_name,
                    carousel_position,
                    None,
                    'show'
                ],
                1
            )

    def get_info(self):
        self.info = merge_info(self.info, self.midterm_info)
        return dict(self.info)
