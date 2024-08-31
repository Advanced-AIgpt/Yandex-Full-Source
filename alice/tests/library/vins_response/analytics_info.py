from cached_property import cached_property


class MegamindAnalyticsInfo(object):
    def __init__(self, analytics_info):
        self._info = analytics_info
        self._scenario_info = self._info.get('scenario_analytics_info')

    @property
    def intent(self):
        return self._scenario_info.get('intent', None) if self._scenario_info else None

    @property
    def product_scenario(self):
        return self._scenario_info.product_scenario_name if self._scenario_info else None

    @property
    def nlg_render_history_records(self):
        return self._scenario_info.get('nlg_render_history_records', None) if self._scenario_info else None

    def object(self, name):
        return self._get_internal_value('objects', name)

    @cached_property
    def objects(self):
        return {_.id: _ for _ in self._scenario_info.objects}

    def event(self, name):
        return self._get_internal_value('events', name)

    @cached_property
    def slots(self):
        return self._info.get('semantic_frame', {}).get('slots', [])

    @cached_property
    def frame_actions(self):
        return self._info.get('frame_actions', {})

    def _get_internal_value(self, value_type_name, value_name):
        for value in self._scenario_info.get(value_type_name, []):
            if value_name in value:
                return value[value_name]
