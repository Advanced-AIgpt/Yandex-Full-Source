# -*- coding: utf-8 -*-

FIXED_PARENT_IDS = [
    'FRONTEND_CATEG_PROMO_MIXED',
    'CATEG_FILM',
    'CATEG_ANIM_FILM',
    'CATEG_SERIES',
    'CATEG_ANIM_SERIES',
    'delayed_tvo',
    'Недавние',
    'Recently used'
]


def get_content_id(event_name, event_value):
    try:
        if event_name == 'carousel_scroll' and 'end_id' in event_value:
            return event_value['end_id']
        elif 'content_id' in event_value:
            return event_value['content_id']
        else:
            return None
    except KeyError:
        return None

def get_parent_id(event_value):
    try:
        if 'parent_id' in event_value:
            return event_value['parent_id']
        else:
            return None
    except KeyError:
        return None


def get_content_type(event_name, event_value):
    try:
        if event_name == 'carousel_scroll' and 'end_type' in event_value:
            return event_value['end_type']
        elif 'content_type' in event_value:
            return event_value['content_type']
        else:
            return None
    except KeyError:
        return None


def get_place(event_value):
    try:
        if 'place' in event_value:
            return event_value['place']
        else:
            return None
    except KeyError:
        return None


def get_content_card_position(event_name, event_value):
    try:
        if event_name == 'carousel_scroll' and 'end_x' in event_value:
            return event_value['end_x']
        elif 'x' in event_value:
            return event_value['x']
        else:
            return None
    except KeyError:
        return None


def get_carousel_position(event_name, event_value):
    try:
        if event_name == 'carousel_scroll' and 'end_y' in event_value:
            return event_value['end_y']
        elif 'y' in event_value:
            return event_value['y']
        else:
            return None
    except KeyError:
        return None


def get_carousel_title(carousel_title, parent_id):
    if carousel_title:
        return carousel_title
    elif parent_id in FIXED_PARENT_IDS:
        return parent_id
    return None
