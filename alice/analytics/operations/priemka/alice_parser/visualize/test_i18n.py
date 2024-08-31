# coding: utf-8

import sys
reload(sys)  # noqa
sys.setdefaultencoding('utf8')  # noqa

import gettext


def load_translations(domain):
    return gettext.translation(domain, localedir='i18n', fallback=True)


t = load_translations('generic_scenario_to_human_readable')
t.install()


def _(text):
    """
    HACK: Обёртка над gettext для python2 строк
    вместо: `_ = t.gettext`
    :param str text:
    """
    return t.gettext(text.decode('utf-8'))


print(_('Настройка будильников'))
