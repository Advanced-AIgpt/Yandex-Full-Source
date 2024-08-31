# coding: utf-8
from __future__ import unicode_literals

from personal_assistant.callback import callback_method
from vins_core.nlu.sample_processors.strip_activation import StripActivationSamplesProcessor


LANE_NAMES_MAP = {
    'left': 'Левый ряд',
    'right': 'Правый ряд',
    'middle': 'Средний ряд',
}


class NaviCallbacksMixin(object):
    WORDS_TO_STRIP_FOR_COMMENTS = [
        'вижу',
        'впереди',
        'добавь',
        'зафиксируй что тут',
        'здесь',
        'можешь отметить',
        'можешь поставить',
        'отметить здесь',
        'отметь что тут',
        'отметь',
        'поставить метку',
        'поставить точку',
        'поставить',
        'поставь',
        'появилась',
        'слушай яндекс',
        'тут',
        'указывай',
        'установить точку',
        'установить',
    ]

    def __init__(self, *args, **kwargs):
        super(NaviCallbacksMixin, self).__init__(*args, **kwargs)
        self._strip_comment_processor = StripActivationSamplesProcessor(
            apply_to_text_input=True,
            custom_front_activations=self.WORDS_TO_STRIP_FOR_COMMENTS,
            custom_back_activations=(),
            min_tokens_after_short_strip=1,
            min_tokens_after_long_strip=1,
        )

    @callback_method
    def navi__add_point__add_comment(self, form, sample, **kwargs):
        if form.road_event.value in (
                'traffic_accidents', 'road_works', 'camera',
                'error_no_route', 'error_no_turn'):
            striped_sample = self._strip_comment_processor(sample)
            comment = striped_sample.text.capitalize()
            form.comment.set_value(comment, 'string')
        elif form.comment.value:
            form.comment.set_value(form.comment.value.capitalize(), 'string')

    @callback_method
    def navi__add_point__update_comment(self, form, sample, **kwargs):
        if form.comment.value and form.road_event.value not in (
                'traffic_accidents', 'road_works', 'camera',
                'error_no_route', 'error_no_turn'):
            form.comment.set_value(form.comment.value.capitalize(), 'string')

    @callback_method
    def navi__add_point__fill_lane(self, form, sample, **kwargs):
        if form.comment.value and form.road_event.value in ('traffic_accidents', 'road_works'):
            comment = '%s. %s' % (form.comment.value, LANE_NAMES_MAP.get(form.lane.value, form.lane.value.capitalize()))
            form.comment.set_value(comment, 'string')
