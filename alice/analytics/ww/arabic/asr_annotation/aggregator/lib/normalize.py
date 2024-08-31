import re
import unicodedata
from collections import Counter, OrderedDict
from num2words import num2words

# copy paste code from https://github.com/linuxscout/tashaphyne/blob/master/tashaphyne/normalize.py
# normalize.py, arabic_const.py
from . import tashaphyne_contrib as normalize_arabic
from . import arabic_const

NORMALIZERS = OrderedDict()


def normalizer(fn):
    NORMALIZERS[fn.__name__] = fn
    def wrapper(*args, **kwargs):
        fn(*args, **kwargs)
    return wrapper


@normalizer
def normalize_lower(text):
    """
    Всё к нижнему регистру
    В английском названия песен

    :param str text:
    :return str:
    """
    return text.lower()


@normalizer
def normalize_unicodedata(text):
    """
    The normal form KD (NFKD) will apply the compatibility decomposition
    https://docs.python.org/3/library/unicodedata.html#unicodedata.normalize

    :param str text:
    :return str:
    """
    return unicodedata.normalize('NFKC', text)


@normalizer
def normalize_digits(text):
    """
    Записать цифры буквами
    TODO: проверить правильность написания цифр

    :param str text:
    :return str:
    """
    return re.sub('\d+', lambda match: f" {num2words(int(match.group(0)), lang='ar')} ", text)  # noqa


def is_text_fully_noise(text):
    """
    Возвращает True если в тексте нет никаких символов кроме вопросиков (SPOKEN NOISE) и пробелов

    :param str text:
    :return bool:
    """
    text = text.replace(arabic_const.SPOKEN_NOISE, '')
    text = text.replace(' ', '')
    return len(text) == 0


@normalizer
def normalize_spn(text):
    """
    Нормализация вопросиков — spoken noise (unknown words)
    ؟ -> ?

    :param str text:
    :return str:
    """
    text = text.replace('؟', arabic_const.SPOKEN_NOISE)
    if is_text_fully_noise(text):
        return 'null'
    return text


@normalizer
def normalize_punctuation(text):
    """
    Убрать знаки препинания

    :param str text:
    :return str:
    """
    return re.sub("[!\"',-.:;`\u060C\u060D\u061B\u061E\u061F]", ' ', text)


@normalizer
def normalize_strip_tashkeel(text):
    """
    Убрать огласовки

    :param str text:
    :return str:
    """
    return normalize_arabic.strip_tashkeel(text)


@normalizer
def normalize_strip_tatweel(text):
    """
    Убрать татвиль — длинный underscore

    :param str text:
    :return str:
    """
    return normalize_arabic.strip_tatweel(text)


@normalizer
def normalize_hamza(text):
    """
    Убирает Hamza над/под Alef
    Оставляет Hamza над Alef_maksura (https://en.wiktionary.org/wiki/ئ)
    и над Wow https://en.wiktionary.org/wiki/ؤ

    :param str text:
    :return str:
    """
    return normalize_arabic.normalize_hamza(text)


@normalizer
def normalize_space(text):
    """
    Пробелы

    :param str text:
    :return str:
    """
    return re.sub(r'[ \s]+', ' ', text).strip()


def patch_ta_marbuta(words):
    """
    Если в самом частом слове есть TaMarbuta на конце, а в других — Ha
    то заменяем в остальных словах Ha на TaMarbuta

    :param List[Tuple[str, int]] words:
    :return List[Tuple[str, int]]:
    """
    votes = Counter()
    for i in range(len(words)):
        votes[words[i][0]] += 1
    results = votes.most_common()

    if results and results[0] and results[0][0].endswith(arabic_const.TEH_MARBUTA):
        for i in range(len(words)):
            if words[i][0].endswith(arabic_const.HEH):
                words[i][0] = words[i][0][:-1] + arabic_const.TEH_MARBUTA
    return words


def patch_alef_maksura(words):
    """
    Если в самом частом слове есть Ya на конце, а в других — Alef Maksura
    то заменяем в остальных словах Alef Maksura на Ya

    :param List[Tuple[str, int]] words:
    :return List[Tuple[str, int]]:
    """

    votes = Counter()
    for i in range(len(words)):
        votes[words[i][0]] += 1
    results = votes.most_common()

    if results and results[0] and results[0][0].endswith(arabic_const.YEH):
        for i in range(len(words)):
            if words[i][0].endswith(arabic_const.ALEF_MAKSURA):
                words[i][0] = words[i][0][:-1] + arabic_const.YEH
    return words


# ===========================================================================================

def normalize_text(text, normalizers='<EMPTY>'):
    """
    Нормализация текста

    :param str text:
    :param str|List normalizers: нормализаторы, которые применять к тексту. По-умолчанию, при <EMPTY> - используются все существующие нормализаторы
    :return str:
    """
    if normalizers == '<EMPTY>':
        normalizers = NORMALIZERS
    if normalizers:
        for name, fn in NORMALIZERS.items():
            text = fn(text)
    return text
