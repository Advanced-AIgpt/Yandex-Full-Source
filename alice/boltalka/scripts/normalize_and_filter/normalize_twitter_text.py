# -*- coding: utf-8 -*-
import re
import string
from HTMLParser import HTMLParser

PUNCT_REGEX = re.compile(u'([' + string.punctuation + ur'\\])')
BAD_CHAR_REGEX = re.compile(u'[^0-9a-zA-Zа-яА-ЯёëЁË \t' + string.punctuation + ur'\\]', flags=re.U)

class TwitNormalizer(object):
    def __init__(self, strip_nonstandard_chars=False, keep_punct=string.punctuation):
        self.strip_nonstandard_chars = strip_nonstandard_chars

        good_punctuation = keep_punct
        self.bad_punctuation = ''.join([p for p in string.punctuation if not p in set(good_punctuation)])
        self.html_parser = HTMLParser()
        self.yo_small_regex = re.compile(u'[ёë]', re.UNICODE)
        self.yo_big_regex = re.compile(u'[ЁË]', re.UNICODE)

    def __call__(self, text):
        text = self.html_parser.unescape(text)
        text = self.yo_big_regex.sub(u'Е', self.yo_small_regex.sub(u'е', text))

        if self.strip_nonstandard_chars:
            text = re.sub(BAD_CHAR_REGEX, '', text)

        # ???? -> ?
        text = re.sub(ur'''([!"#$%&'()*+,\-/:;<=>?@[\]\^_`{|}~])\1+''', ur'\1', text, flags=re.UNICODE)
        text = re.sub(ur'''([!"#$%&'()*+,\-/:;<=>?@[\]\^_`{|}~] )\1+''', ur'\1', text, flags=re.UNICODE)
        text = re.sub(ur'''[.]{4,}''', ur'...', text, flags=re.UNICODE)
        text = re.sub(ur'''(.)\1{3,}''', ur'\1\1\1', text, flags=re.UNICODE)
        text = ' ' + text + ' '
        text = re.sub(ur'''(?<=[^a-zA-Zа-яА-Я0-9])[хаХАxaXA]{3}(?=[^a-zA-Zа-яА-Я0-9])''', ur'ха', text, flags=re.UNICODE)
        text = re.sub(ur'''(?<=[^a-zA-Zа-яА-Я0-9])[хаХАxaXA]{4,}(?=[^a-zA-Zа-яА-Я0-9])''', ur'ха', text, flags=re.UNICODE)
        text = re.sub(ur'''[Aa]+[wW]+''', ' ', text)

        text = re.sub(ur'\s+', ' ', text, flags=re.UNICODE).strip()
        return text

def separate_punctuation(s):
    s = re.sub(PUNCT_REGEX, ur' \1 ', s).strip()
    s = re.sub(ur'\s+', ' ', s, flags=re.U).strip()
    return s

if __name__ == '__main__':
    import sys
    import codecs
    import argparse
    sys.stdin = codecs.getreader('utf-8')(sys.stdin)
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout)

    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--strip-nonstandard-chars', action='store_true')
    parser.add_argument('--keep-punct', default=string.punctuation, help='all punctuation by default')
    args = parser.parse_args()

    prpr = TwitNormalizer(args.strip_nonstandard_chars, args.keep_punct)

    for line in sys.stdin:
        print prpr(line.strip())
