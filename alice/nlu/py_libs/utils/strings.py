# coding: utf-8

from __future__ import unicode_literals, absolute_import

from transliterate import translit


def smart_unicode(s):
    if isinstance(s, str):
        return s.decode('utf-8')
    return unicode(s)


def is_ascii(s):
    try:
        s.decode('ascii')
        return True
    except UnicodeDecodeError:
        return False


def fix_protobuf_string(s):
    if isinstance(s, unicode):
        return s

    elif is_ascii(s):
        return s

    else:
        return smart_unicode(s)


def smart_utf8(s):
    if isinstance(s, unicode):
        return s.encode('utf-8')
    return str(s)


def utf8call(function, string, **kwargs):
    return function(string.encode('utf-8'), **kwargs).decode('utf-8')


def isnumeric(string):
    digits = set('0123456789')
    return all(sym in digits for sym in string)


def get_translit(text):
    return translit(text, 'ru', reversed=False)
