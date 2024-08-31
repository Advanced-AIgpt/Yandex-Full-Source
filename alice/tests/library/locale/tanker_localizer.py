class TankerLocalizer(object):
    def __init__(self, tanker_data, locale_name, test_group_id):
        assert test_group_id in tanker_data.get(locale_name, {}), \
            f'Failed to find translations for locale "{locale_name}" and test_group "{test_group_id}"'
        self._translations = tanker_data[locale_name][test_group_id]

    def localize_command(self, command):
        assert command in self._translations, f'Can not find translation for command "{command}"'
        return self._translations[command]
