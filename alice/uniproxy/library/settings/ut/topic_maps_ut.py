from alice.uniproxy.library.settings import TOPIC_MAPS, LANG_MAPS


def test_topic_mapping():
    assert not TOPIC_MAPS.errors
    assert TOPIC_MAPS.get('mapsyari', 'ru') == 'mapsyari'
    assert TOPIC_MAPS.get('mapsyari', 'en-TR') == 'mapsyari'
    assert TOPIC_MAPS.get('mapsyari', 'en-EN') == 'maps'
    assert TOPIC_MAPS.get('mapsyari', '__-__') == 'mapsyari'
    assert TOPIC_MAPS.get('maps', 'en-TR') == 'mapsyari'
    assert TOPIC_MAPS.get('maps', 'en-EN') == 'maps'
    assert TOPIC_MAPS.get('queries', 'ru-RU') == 'general'
    assert TOPIC_MAPS.get('dialog-general', '-') == 'dialogeneral'
    assert TOPIC_MAPS.get('counters', 'en-TR') == 'dialogeneralfast'


def test_lang_mapping():
    assert not LANG_MAPS.errors
    assert LANG_MAPS.get('ru') == 'ru-RU'
    assert LANG_MAPS.get('__-__') == 'ru-RU'
    assert LANG_MAPS.get('en-TR') == 'tr-TR'
    assert LANG_MAPS.get('uk-UK') == 'uk-UA'
    assert LANG_MAPS.get('en-ES') == 'en-EN'
