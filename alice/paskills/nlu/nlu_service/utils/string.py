# coding: utf-8


def ensure_unicode(string):
    if isinstance(string, unicode):
        return string
    else:
        return string.decode('utf-8')


def smart_utf8(s):
    if isinstance(s, unicode):
        return s.encode('utf-8')
    return str(s)
