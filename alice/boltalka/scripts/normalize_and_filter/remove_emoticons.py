#!/usr/bin/python
# coding=utf-8
import sys, argparse, string, codecs
import re
#import regex as re
from normalize_twitter_text import TwitNormalizer

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
    def __init__(self, strip_nonstandard_chars=False, keep_punct=string.punctuation):
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
            #ur'''(?<=\s)[dDдД]*([xXхХ]+[dDдД]*)+(?=\s)''',
            ur'''(?<=\s)[xXхХ]+[dDдД]*(?=\s)''',

            # :p
            ur'''(?<=\s|[^0-9])>?[:=;\-\^8*]+['"]*([()\[\]{}/\\*<>])\1*''',
            #ur'''(?<=^|\s|[^0-9])>?[:=;\-\^8'*]+([bвВBdDpPрРдДзsSoOоОхХxXcCсС])\1*(?=$|\s)''',
            ur'''(?<=\s|[^0-9])>?[:=;\-\^8*]+['"]*([bвВBdDpPрРдДзsS])\1*(?=\s)''',
            ur'''(?<=\s|[^0-9])>?[:=;\-\^8*]+['"]*([oOоОхХxXcCсС])\1*(?=\s)''',
            ur'''([()\[\]{}/\\*<>])\1*['"]*[:=;\-\^8*]+<?(?=\s|[^0-9])''',
            #ur'''(?<=^|\s)([bвВBdDpPрРдДзsSoOоОхХxXcCсС])\1*[:=;\-\^8'*]+<?(?=$|\s|[^0-9])''',
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

            #!smile!
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

if __name__ == '__main__':
    sys.stdin = codecs.getreader('utf-8')(sys.stdin)
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout)

    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--strip-nonstandard-chars', action='store_true')
    parser.add_argument('--keep-punct', default=string.punctuation, help='all punctuation by default')
    args = parser.parse_args()

    prpr = EmoticonsRemover(args.strip_nonstandard_chars, args.keep_punct)
    s = prpr(u'привет! 8]]] 8[[ [[8 ]]8 8( )8 :((( ))-: 8-( хай? ))) ^^ ^___^ т__T 0_o что это?:D:D пыщ._ .', iterative=True)
    t = u'привет! хай? что это? пыщ'
    if s != t:
        print len(s), s
        print len(t), t
        exit(1)

    for line in sys.stdin:
        print prpr(line.strip(), iterative=True)
    """
    while True:
        text = raw_input('> ')
        print prpr(text), '\n'
    """

