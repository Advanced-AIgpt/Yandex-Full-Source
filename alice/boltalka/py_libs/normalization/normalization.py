# coding=utf-8
import re
import string
from HTMLParser import HTMLParser

PUNCT_REGEX = re.compile(u'([' + string.punctuation + ur'\\])')
BAD_CHAR_REGEX = re.compile(
    u'[^0-9a-zA-Zа-яА-ЯёëЁË \t' + string.punctuation + ur'\\]', flags=re.U)


def separate_punctuation(s):
    s = re.sub(PUNCT_REGEX, ur' \1 ', s).strip()
    s = re.sub(ur'\s+', ' ', s, flags=re.U).strip()
    return s


def tokenize(s):
    return separate_punctuation(s).split()


def normalize(s):
    return ' '.join(re.sub(ur'[^а-я\s0-9?!]', ' ', s.strip()).split()).strip()


class TwitNormalizer(object):
    def __init__(self,
                 strip_nonstandard_chars=False,
                 keep_punct=string.punctuation):
        self.strip_nonstandard_chars = strip_nonstandard_chars

        good_punctuation = keep_punct
        self.bad_punctuation = ''.join(
            [p for p in string.punctuation if p not in set(good_punctuation)])
        self.html_parser = HTMLParser()
        self.yo_small_regex = re.compile(u'[ёë]', re.UNICODE)
        self.yo_big_regex = re.compile(u'[ЁË]', re.UNICODE)

    def __call__(self, text):
        text = self.html_parser.unescape(text)
        text = self.yo_big_regex.sub(u'Е', self.yo_small_regex.sub(u'е', text))

        if self.strip_nonstandard_chars:
            text = re.sub(BAD_CHAR_REGEX, '', text)

        # ???? -> ?
        text = re.sub(
            ur'''([!"#$%&'()*+,\-/:;<=>?@[\]\^_`{|}~])\1+''',
            ur'\1',
            text,
            flags=re.UNICODE)
        text = re.sub(
            ur'''([!"#$%&'()*+,\-/:;<=>?@[\]\^_`{|}~] )\1+''',
            ur'\1',
            text,
            flags=re.UNICODE)
        text = re.sub(ur'''[.]{4,}''', ur'...', text, flags=re.UNICODE)
        text = re.sub(ur'''(.)\1{3,}''', ur'\1\1\1', text, flags=re.UNICODE)
        text = ' ' + text + ' '
        text = re.sub(
            ur'''(?<=[^a-zA-Zа-яА-Я0-9])[хаХАxaXA]{3}(?=[^a-zA-Zа-яА-Я0-9])''',
            ur'ха',
            text,
            flags=re.UNICODE)
        text = re.sub(
            ur'''(?<=[^a-zA-Zа-яА-Я0-9])[хаХАxaXA]{4,}(?=[^a-zA-Zа-яА-Я0-9])''',
            ur'ха',
            text,
            flags=re.UNICODE)
        text = re.sub(ur'''[Aa]+[wW]+''', ' ', text)

        text = re.sub(ur'\s+', ' ', text, flags=re.UNICODE).strip()
        return text


def remove_unbalanced_braces(s):
    openning = u'([{'
    closing = u')]}'
    match = dict(zip(openning, closing))
    stack = [None] * len(s)
    stack_top = 0
    delete = [False] * len(s)
    for i, c in enumerate(s):
        if c in openning:
            stack[stack_top] = (c, i)
            stack_top += 1
            continue
        if c in closing:
            if stack_top == 0 or match[stack[stack_top - 1][0]] != c:
                delete[i] = True
                continue
            stack_top -= 1

    while stack_top > 0:
        c, i = stack[stack_top - 1]
        stack_top -= 1
        delete[i] = True

    return u''.join(c for i, c in enumerate(s) if not delete[i])


class EmoticonsRemover(object):
    def __init__(self,
                 strip_nonstandard_chars=False,
                 keep_punct=string.punctuation):
        self.normalizer = TwitNormalizer(strip_nonstandard_chars, keep_punct)

        regexes = [
            # *____*
            ur'''([\^])+[_\-=oOоО0~]*\1*''',
            ur'''(-|–)+[_\-=oOоО0~]+\1+''',
            ur'''([*])+[_\-=oOоО0~]+\1+''',
            ur'''>[_\-=oOоО0]+<''',
            ur'''<[_\-=oOоО0]+>''',
            ur'''(?<=\s)[тТT][–\-_]+[тТT](?=\s)''',
            ur'''(?<=\s)[oOоО0qQ] ?[–\-_.]+ ?[oOоО0qQ](?=\s)''',
            ur'''[.] ?[–\-_]+ ?[.]''',

            # \o/
            ur'''([/\\])* ?[oOоО] ?(?(1)|[/\\])[/\\]*''',
            ur'''([/\\])* ?0 ?(?(1)|[/\\])[/\\]*(?! [0-9])''',
            ur'''([/\\])* ?0 ?(?(1)|[/\\])[/\\]*(?![0-9])''',

            # хдд ххххх
            ur'''(?<=\s)[xXхХ]+[dDдД]*(?=\s)''',

            # :p
            ur'''(?<=\s|[^0-9])>?[:=;\-\^8*]+['"]*([()\[\]{}/\\*<>])\1*''',
            ur'''(?<=\s|[^0-9])>?[:=;\-\^8*]+['"]*([bвВBdDpPрРдДзsS])\1*(?=\s)''',
            ur'''(?<=\s|[^0-9])>?[:=;\-\^8*]+['"]*([oOоОхХxXcCсС])\1*(?=\s)''',
            ur'''([()\[\]{}/\\*<>])\1*['"]*[:=;\-\^8*]+<?(?=\s|[^0-9])''',
            ur'''(?<=\s)([bвВBdDpPрРдДзsS])\1*['"]*[:=;\-\^8*]+<?(?=\s|[^0-9])''',
            ur'''(?<=\s)([oOоОхХxXcCсС])\1*['"]*[:=;\-\^8*]+<?(?=\s|[^0-9])''',

            # :p
            ur'''(?<=\s|[^0-9])>?[:=;\-\^*]+['"]*([3])\1*''',
            ur'''(?<=\s|[^0-9])>?[:=;\-\^*]+['"]*([0])\1*(?=\s)''',
            ur'''([3])\1*['"]*[:=;\-\^*]+<?(?=\s|[^0-9])''',
            ur'''(?<=\s)([0])\1*['"]*[:=;\-\^*]+<?(?=\s|[^0-9])''',

            # !!1!! 1 !!! -> !!!!!!
            ur'''((?<=!) ?1+|1+ ?(?=!))''',
            # 00)))))))00 -> )))))
            ur'''((?<=[)(]) ?[09]+|[09]+ ?(?=[)(]))''',

            # .!.
            ur'''\.\s*[!/\\|]+\s*\.''',
            ur'''\. (?:[!\\/|] )+\.''',
            ur'''(?<=\s)о (?:[!\\/|] )+о(?=\s|[^0-9])''',

            # <3
            r'''<+[3з]+''',

            # !smile!
            r'''![a-z0-9]+!'''
        ]
        self.regexes = []
        for r in regexes:
            self.regexes.append(re.compile(r, flags=re.UNICODE))

    def __call__(self, text, is_normalized=False, iterative=False):
        while True:
            prev = text
            if not is_normalized:
                text = self.normalizer(text)

            for r in self.regexes:
                text = ' ' + text + ' '
                text = re.sub(r, ' ', text).strip()

            text = remove_unbalanced_braces(text)
            text = text.lstrip(ur'''!'+,-./:;?@[\]^_`{|}~''')
            text = text.rstrip(ur'''#&*+,\-/:;<=>@[\]\^_`{|}~q''')
            # [ 2 ]
            text = re.sub(ur'''\[[\s0-9]*\]''', ' ', text)
            # ( ) ( 1 )
            text = re.sub(ur'''\([\s0-9]*\)''', ' ', text)
            text = text.replace('*', ' ')
            if text.count('"') == 1:
                text = text.replace('"', ' ')

            text = re.sub(ur'\s+', ' ', text, flags=re.UNICODE).strip()
            if prev == text:
                break
        return text
