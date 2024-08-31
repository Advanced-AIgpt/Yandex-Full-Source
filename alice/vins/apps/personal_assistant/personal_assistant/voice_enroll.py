# coding: utf-8


def count_syllables(message):
    if message is None:
        return 0
    vowels = set([u'а', u'е', u'ё', u'и', u'о', u'у', u'ы', u'э', u'ю', u'я'])
    return len([x for x in message if x in vowels])
