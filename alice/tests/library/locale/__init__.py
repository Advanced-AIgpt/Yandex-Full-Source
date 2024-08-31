from functools import wraps

from .locale import Locale


def _wraps_locale(locale_func):
    @wraps(locale_func)
    def wrapper(**settings):
        @wraps(locale_func)
        def locale_partial():
            return locale_func(**settings)
        return locale_partial
    return wrapper


@_wraps_locale
def ru_ru():
    return Locale('ru-RU')


@_wraps_locale
def ar_sa(use_tanker=True):
    result = Locale('ar-SA')
    result.experiments = {
        'mm_allow_lang_ar': '1',
    }
    result.use_tanker = use_tanker
    return result


actual_locales = [ru_ru, ar_sa]


def is_ru(alice):
    return alice.application.Lang.startswith('ru')
