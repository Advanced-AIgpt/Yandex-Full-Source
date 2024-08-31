import re
import string
import html

BAD_CHAR_REGEXES = {
    'ru': re.compile('[^0-9a-zA-Zа-яА-ЯёëЁË \t' + string.punctuation + r'\\]', flags=re.U),
    'tr': re.compile('[^0-9a-zA-ZığüşöçİĞÜŞÖÇâîû \t' + string.punctuation + r'\\]', flags=re.U)
}


def lowercase_fix(text, lang):
    if lang == 'tr':
        return text.replace('I', 'ı').lower()
    return text.lower()


def uppercase_first_fix(text, lang):
    if text is None or len(text) == 0:
        return text
    first_letter = text[0].upper()
    if lang == 'tr':
        first_letter = text[0].replace('i', 'İ').upper()
    return first_letter + text[1:]


class TextNormalizer(object):
    ''' prepare dataset (twitter, librusec, logs, etc.) for training '''
    def __init__(self, args):
        self.strip_bad_chars = args.strip_bad_chars
        self.separate_punctuation = args.separate_punctuation
        self.remove_punctuation = args.remove_punctuation
        self.lowercase = args.lowercase
        self.uppercase_first = args.uppercase_first
        self.replace_yo = args.replace_yo
        self.strip_ext = args.strip_ext
        self.skip_140 = args.skip_140
        self.skip_numeric_many = args.skip_numeric_many
        self.regex_separate = re.compile('([' + string.punctuation + r'\\])', re.UNICODE)
        self.regex_yo_small = re.compile('[ёë]', re.UNICODE)
        self.regex_yo_big = re.compile('[ЁË]', re.UNICODE)
        self.regex_bad_chars = BAD_CHAR_REGEXES[args.lang]
        self.regex_140 = re.compile(r'(?:^|[^0-9])140[^0-9]|[^0-9]140(?:$|[^0-9])')
        self.lang = args.lang

    def __call__(self, text):
        if text is None:
            return

        if self.separate_punctuation:
            text = self.regex_separate.sub(r' \1 ', text)
        if self.remove_punctuation:
            text = self.regex_separate.sub('', text)
        if self.lowercase:
            text = lowercase_fix(text, self.lang)
        if self.uppercase_first:
            text = uppercase_first_fix(text, self.lang)
        if self.replace_yo:
            text = self.regex_yo_big.sub('Е', self.regex_yo_small.sub('е', text))
        if self.strip_bad_chars:
            text = re.sub(self.regex_bad_chars, '', text)
        if self.strip_ext:
            text = re.sub(r'''([!"#$%&'()*+,\-/:;<=>?@[\]\^_`{|}~])\1+''', r'\1', text, flags=re.UNICODE)
            text = re.sub(r'''([!"#$%&'()*+,\-/:;<=>?@[\]\^_`{|}~] )\1+''', r'\1', text, flags=re.UNICODE)
            text = re.sub(r'''[.]{4,}''', r'...', text, flags=re.UNICODE)
            text = re.sub(r'''(.)\1{3,}''', r'\1\1\1', text, flags=re.UNICODE)
            text = ' ' + text + ' '
            text = re.sub(r'''(?<=[^a-zA-Zа-яА-Я0-9])[хаХАxaXA]{3}(?=[^a-zA-Zа-яА-Я0-9])''', r'ха', text, flags=re.UNICODE)
            text = re.sub(r'''(?<=[^a-zA-Zа-яА-Я0-9])[хаХАxaXA]{4,}(?=[^a-zA-Zа-яА-Я0-9])''', r'ха', text, flags=re.UNICODE)
            text = re.sub(r'''[Aa]+[wW]+''', ' ', text)
        if self.skip_140 and self.regex_140.search(text):
            return
        if self.skip_numeric_many and len(re.findall('[0-9]', text)) > 3:
            return
        text = re.sub('\s+', ' ', text, flags=re.UNICODE).strip()

        return text

    @staticmethod
    def add_args(parser):
        parser.add_argument('--lang', choices=['ru', 'tr'], default='ru')
        parser.add_argument('--strip-bad-chars', action='store_true')
        parser.add_argument('--separate-punctuation', action='store_true')
        parser.add_argument('--remove-punctuation', action='store_true')
        parser.add_argument('--lowercase', action='store_true')
        parser.add_argument('--uppercase-first', action='store_true')
        parser.add_argument('--replace-yo', action='store_true')
        parser.add_argument('--strip-ext', action='store_true')
        parser.add_argument('--skip-140', action='store_true')
        parser.add_argument('--skip-numeric-many', action='store_true')


class TwitterCleaner(object):
    ''' clean out raw twitter dataset '''
    def __init__(self, args):
        self.skip_rt = args.skip_rt
        self.skip_url = args.skip_url
        self.skip_hashtag = args.skip_hashtag
        self.skip_new_post = args.skip_new_post
        self.strip_mention = args.strip_mention
        self.skip_at = args.skip_at
        self.replace_html_entities = args.replace_html_entities
        self.regex_rt = re.compile('(^|\s)RT\s', re.UNICODE)
        self.regex_url = re.compile('(https?://|twitter)', re.UNICODE)
        self.regex_hashtag = re.compile('#[^\s]+', re.UNICODE)
        self.regex_mention = re.compile('(^|\s+)@[^\s]+', re.UNICODE)

    def __call__(self, text):
        if text is None:
            return

        if self.skip_rt and self.regex_rt.search(text):
            return
        if self.skip_url and self.regex_url.search(text):
            return
        if self.skip_hashtag and self.regex_hashtag.search(text):
            return
        if 'New post: ' in text:  # most of them are ads
            return
        if self.strip_mention:
            text = self.regex_mention.sub('', text)
        if self.replace_html_entities:
            text = html.unescape(text)
        if self.skip_at and '@' in text:
            return
        return text

    @staticmethod
    def add_args(parser):
        parser.add_argument('--skip-rt', action='store_true')
        parser.add_argument('--skip-url', action='store_true')
        parser.add_argument('--skip-hashtag', action='store_true')
        parser.add_argument('--skip-new-post', action='store_true')
        parser.add_argument('--strip-mention', action='store_true')
        parser.add_argument('--skip-at', action='store_true')
        parser.add_argument('--replace-html-entities', action='store_true')


class LogsCleaner(object):
    def __init__(self, args):
        self.speaker_regex = re.compile(r'<speaker (voice|speed)=.+?>', re.UNICODE)
        self.music_regex = re.compile(r'<\[/?domain( music)?\]>')
        self.pause_regex = re.compile(r'(\.sil<\[\d+\]>|<\[.+?\]>)', re.UNICODE)
        self.search_regex = re.compile(r' \[ Открывается поиск по запросу <.*?> \]', re.UNICODE)

    def __call__(self, text):
        if text is None or text == 'EMPTY':
            return ''
        text = self.speaker_regex.sub('', text)
        text = self.pause_regex.sub('', text)
        text = self.search_regex.sub('', text)
        text = html.unescape(text)
        text = re.sub('\s+', ' ', text).strip()
        return text

    @staticmethod
    def add_args(parser):
        pass


class PunctEncoder(object):
    ''' encode punctuation for insight's dssm '''
    def __init__(self, args):
        punctuation = string.punctuation
        self.mapping = {}
        for i, c in enumerate(punctuation):
            self.mapping[c] = chr(ord('A') + i % 26)
            if i >= 26:
                self.mapping[c] += '0'

    def __call__(self, text):
        if text is None:
            return

        return ''.join([self.mapping.get(c, c) for c in text])

    @staticmethod
    def add_args(parser):
        pass


def remove_unbalanced_braces(s):
    openning = '([{'
    closing = ')]}'
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

    return ''.join(c for i, c in enumerate(s) if not delete[i])


class EmoticonsRemover(object):
    def __init__(self, strip_nonstandard_chars=False, keep_punct=string.punctuation):
        regexes = [
            # *____*
            r'''([\^])+[_\-=oOоО0~]*\1*''',
            r'''(-|–)+[_\-=oOоО0~]+\1+''',
            r'''([*])+[_\-=oOоО0~]+\1+''',
            r'''>[_\-=oOоО0]+<''',
            r'''<[_\-=oOоО0]+>''',
            r'''(?<=\s)[тТT][–\-_]+[тТT](?=\s)''',
            r'''(?<=\s)[oOоО0qQ] ?[–\-_.]+ ?[oOоО0qQ](?=\s)''',
            r'''[.] ?[–\-_]+ ?[.]''',

            # \o/
            r'''([/\\])* ?[oOоО] ?(?(1)|[/\\])[/\\]*''',
            r'''([/\\])* ?0 ?(?(1)|[/\\])[/\\]*(?! [0-9])''',
            r'''([/\\])* ?0 ?(?(1)|[/\\])[/\\]*(?![0-9])''',

            # хдд ххххх
            r'''(?<=\s)[xXхХ]+[dDдД]*(?=\s)''',

            # :p
            r'''(?<=\s|[^0-9])>?[:=;\-\^8*]+['"]*([()\[\]{}/\\*<>])\1*''',
            r'''(?<=\s|[^0-9])>?[:=;\-\^8*]+['"]*([bвВBdDpPрРдДзsS])\1*(?=\s)''',
            r'''(?<=\s|[^0-9])>?[:=;\-\^8*]+['"]*([oOоОхХxXcCсС])\1*(?=\s)''',
            r'''([()\[\]{}/\\*<>])\1*['"]*[:=;\-\^8*]+<?(?=\s|[^0-9])''',
            r'''(?<=\s)([bвВBdDpPрРдДзsS])\1*['"]*[:=;\-\^8*]+<?(?=\s|[^0-9])''',
            r'''(?<=\s)([oOоОхХxXcCсС])\1*['"]*[:=;\-\^8*]+<?(?=\s|[^0-9])''',

            # :p
            r'''(?<=\s|[^0-9])>?[:=;\-\^*]+['"]*([3])\1*''',
            r'''(?<=\s|[^0-9])>?[:=;\-\^*]+['"]*([0])\1*(?=\s)''',
            r'''([3])\1*['"]*[:=;\-\^*]+<?(?=\s|[^0-9])''',
            r'''(?<=\s)([0])\1*['"]*[:=;\-\^*]+<?(?=\s|[^0-9])''',

            # !!1!! 1 !!! -> !!!!!!
            r'''((?<=!) ?1+|1+ ?(?=!))''',
            # 00)))))))00 -> )))))
            r'''((?<=[)(]) ?[09]+|[09]+ ?(?=[)(]))''',

            # .!.
            r'''\.\s*[!/\\|]+\s*\.''',
            r'''\. (?:[!\\/|] )+\.''',
            r'''(?<=\s)о (?:[!\\/|] )+о(?=\s|[^0-9])''',

            # <3
            r'''<+[3з]+''',

            # !smile!
            r'''![a-z0-9]+!'''
        ]
        self.regexes = []
        for r in regexes:
            self.regexes.append(re.compile(r, flags=re.UNICODE))

    def __call__(self, text):
        if text is None:
            return

        while True:
            prev = text
            for r in self.regexes:
                text = ' ' + text + ' '
                text = re.sub(r, ' ', text).strip()

            text = remove_unbalanced_braces(text)
            text = text.lstrip(r'''!'+,-./:;?@[\]^_`{|}~''')
            text = text.rstrip(r'''#&*+,\-/:;<=>@[\]\^_`{|}~q''')
            # [ 2 ]
            text = re.sub(r'''\[[\s0-9]*\]''', ' ', text)
            # ( ) ( 1 )
            text = re.sub(r'''\([\s0-9]*\)''', ' ', text)
            text = text.replace('*', ' ')
            if text.count('"') == 1:
                text = text.replace('"', ' ')

            text = re.sub(r'\s+', ' ', text, flags=re.UNICODE).strip()
            if prev == text:
                break
        return text

    @staticmethod
    def add_args(parser):
        pass
