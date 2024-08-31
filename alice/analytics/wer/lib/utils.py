import numpy as np
import re
from collections import defaultdict
from library.python import resource
from pymorphy2 import MorphAnalyzer
from typing import Callable, Dict, Iterable, List, Mapping, Optional, Sequence, Set, Tuple, Union
from weighted_levenshtein import levenshtein

from alice.analytics.wer.lib.pos_aware_levenshtein import pos_aware_levenshtein

TAGS = {
    "SPN": "?",
    "EOS": ";"
}

VOWELS = 'aoieyuAOIEYU'
CONSONANTS = 'bcdfghjklmnpqrstvwxzBCDFGHJKLMNPQRSTVWXZ'
PAIRS = [('b', 'p'), ('v', 'w'), ('z', 's'), ('d', 't'), ('g', 'k'),
         ('o', 'O'), ('u', 'o'), ('i', 'y'), ('O', 'u'), ('i', 'j'),
         ('N', 'n'), ('Z', 'S'), ('c', 'k')]
N_CHARS = 128  # константа библиотеки levenshtein, все элементы в строках должны быть ord < 128

# слова с огромной стоимостью вставок/удалений
STOP_WORDS = {'не', 'ни'}
# слова НЕ с огромной стоимостью замен друг на друга (чтобы они не воспринимались как две огромные операции)
FINE_WORD_MATCHING = {('не', 'ни')}
# префиксы с огромной стоимостью замен друг на друга
BAD_PREFIX_MATCHING = {('в', 'вы'), ('у', 'при'), ('под', 'от'), ('подо', 'ото'),
                       ('при', 'от'), ('за', 'раз'), ('в', 'от')}
# префиксы с огромной стоимостью вставок/удалений
BAD_PREFIX_DIFF = ['не', 'ни']

INF = 1e9


def build_substitute_costs(
    cons_cost: float = 1, vowel_cost: float = 1, pair_cost: float = 1, other_cost: float = 1
) -> np.ndarray:
    """
    Возвращает массив стоимостей замен фонем для расстояния Левенштейна
    :param const_cost: стоимость замены согласной на согласную
    :param vowel_cost: стоимость замены гласной на гласную
    :param pair_cost: стоимость замены фонемы на её парную
    :param other_cost: стоимость других замен
    """
    substitute_costs = np.ones((N_CHARS, N_CHARS), dtype=np.float64) * other_cost

    for vowel in VOWELS:
        substitute_costs[ord(vowel), ord(vowel)] = 0
        for vowel2 in VOWELS:
            if vowel != vowel2:
                substitute_costs[ord(vowel), ord(vowel2)] = vowel_cost  # штраф за перепутать гласные

    for consonant in CONSONANTS:
        substitute_costs[ord(consonant), ord(consonant)] = 0
        for consonant2 in CONSONANTS:
            if consonant != consonant2:
                substitute_costs[ord(consonant), ord(consonant2)] = cons_cost  # штраф за перепутать согласные

    for pair in PAIRS:
        substitute_costs[ord(pair[0]), ord(pair[1])] = pair_cost  # штраф за перепутать парные согласные
        substitute_costs[ord(pair[1]), ord(pair[0])] = pair_cost  # в обе стороны
    return substitute_costs


def build_insert_costs(cons_cost: float = 1, vowel_cost: float = 1, other_cost: float = 1) -> np.array:
    """
    Возвращает массив стоимостей вставок/удалений фонем для расстояния Левенштейна
    (вставка == удаление из симметричности)
    :param const_cost: стоимость вставки согласной
    :param vowel_cost: стоимость вставки гласной
    :param other_cost: стоимость других вставок
    """
    insert_costs = np.ones(N_CHARS, dtype=np.float64) * other_cost

    for vowel in VOWELS:
        insert_costs[ord(vowel)] = vowel_cost  # штраф за вставить гласные

    for consonant in CONSONANTS:
        insert_costs[ord(consonant)] = cons_cost  # штраф за вставить согласные
    return insert_costs


def get_phoneme_mapping() -> Mapping[str, str]:
    """Выкачивает словарь первоначального маппинга фонем"""
    phoneme_mapping = {}
    for line in resource.find('/phonemes_mapping.txt').decode('utf-8').split('\n'):
        if '\t' not in line:
            continue
        phoneme_g2p, phoneme_normalized = line.split('\t')
        phoneme_mapping[phoneme_g2p] = phoneme_normalized
    return phoneme_mapping


def remove_unknown_chars(text: str) -> Tuple[str, Sequence[str]]:
    """
    Убирает из фонемного представления "нетипичные" символы (все, кроме пробелов и латиницы)
    :param text: фонемное представление
    :return: очищенное фонемное представление и список удалённых символов
    """
    unknown_chars = re.findall('[^a-zA-Z ]', text)
    clean_text = re.sub('[^a-zA-Z ]+', '', text)
    return clean_text, unknown_chars


def remove_tags(text: str) -> str:
    """
    Убирает из оригинального текста теги (паузы, вопросики)
    :param text: оригинальный текст
    :return: очищенный текст
    """
    tags_regexp = '|'.join(['<{}>'.format(i) for i in TAGS.keys()])
    return re.sub(tags_regexp, '', text)


# @lru_cache(maxsize=1024)
def get_pher(word1: str, word2: str, substitute_costs: np.ndarray, insert_costs: np.array, pos_aware: bool = False) -> float:
    """
    Возвращает пофонемный WERP (PhER)
    """
    return calc_norm_distance(word1, word2, substitute_costs, insert_costs, pos_aware)


def get_weighted_distance(
    ref_words: Sequence[str], hyp_words: Sequence[str], substitute_costs: np.ndarray, insert_costs: np.array,
    SUB_C: float = 1.5, INS_C: float = 1.4, INS_DENOM: float = 12, STICK_EPS: float = 0.1, STICK_C: float = 2.,
    STICK_BIAS: float = 0.1, use_pos_aware_pher: bool = False
) -> float:
    """
    Вычисляет пословное расстояние Левенштейна между представлениями предложений (для WERPv2).
    Операции: вставить/удалить слово, заменить слово на другое, заменить два соседних слова на другое (склейка).

    :param ref_words: список слов из первой строки
    :param ref_words: список слов из второй строки
    :param SUB_C: множитель для стоимости замены
    :param INS_C: множитель для стоимости вставки
    :param INS_DENOM: знаменатель для стоимости вставки
    :param STICK_EPS: порог для активации склейки (четвёртая операция)
    :param STICK_C: множитель для стоимости склейки
    :param use_pos_aware_pher: указывает, подстраивать ли веса операций в PhER под их позицию в слове
    """
    def _ins_cost(s: str) -> float:
        return INS_C * (1 + len(s)) / INS_DENOM

    def _sub_cost(s: str, t: str) -> float:
        return SUB_C * get_pher(s, t, substitute_costs, insert_costs, use_pos_aware_pher)

    def _stick_cost(s: str, t: str) -> float:
        pher = get_pher(s, t, substitute_costs, insert_costs, use_pos_aware_pher)
        if pher < STICK_EPS:
            return STICK_BIAS + STICK_C * pher
        return INF

    dist, _ = align_texts(ref_words, hyp_words,
                          insert_cost_func=_ins_cost, substitute_cost_func=_sub_cost, stick_cost_func=_stick_cost)
    return dist


def normalize_phonemes(phoneme_mapping: Mapping[str, str], phonemes_list: Sequence[str]) -> Sequence[str]:
    """Реализует маппинг из сырых выдач g2p в фонемные представления для WERP"""
    for i, phonemes in enumerate(phonemes_list):
        phonemes_normalized = ''
        for p in phonemes.split():
            ps = p.strip('ʲ:ː+')
            if ps in phoneme_mapping:
                phonemes_normalized += phoneme_mapping[ps]
            else:
                phonemes_normalized += ps
        phonemes_list[i] = phonemes_normalized
    return phonemes_list


def weigh_phonemes(phonemes_list: Sequence[str], score_list: Sequence[float]) -> Mapping[str, float]:
    """Суммирует скоры для одинаковых после маппинга строк"""
    phonemes_dict = defaultdict(float)
    for phonemes, score in zip(phonemes_list, score_list):
        phonemes_dict[phonemes] += max(0, 10 + score)  # все скоры отрицательные; хорошие близки к нулю
    return phonemes_dict


def normalize_words(morph: MorphAnalyzer, text: str) -> str:
    """Приводит слова в тексте text к нормальной форме (инфинитив, имен. падеж)"""
    res = []
    for word in text.split():
        word_parsed = morph.parse(word)
        max_score = 0
        for form in word_parsed:
            if form.score > max_score:
                max_score = form.score
                chosen_form = form
        res.append(chosen_form.normal_form)
    return ' '.join(res)


def get_words(text: str) -> Sequence[str]:
    """
    Возвращает список слов в строке text.
    Тег <unk> из гипотез считается словом unk
    """
    return re.findall(r'[\w\?]+', text)


def calc_distance(
    text1: str,
    text2: str,
    substitute_costs: Optional[np.ndarray] = None,
    insert_costs: Optional[np.array] = None,
    pos_aware: bool = False
) -> float:
    """
    Вычисляет расстояние Левенштейна между строками
    :param text1: строка, str
    :param text2: строка, str
    :param substitute_costs: веса замен символов, np.ndarray размера 128х128, optional
    :param insert_costs: веса вставок/удалений символов, np.array размера 128, optional
    """
    if substitute_costs is None:
        substitute_costs = np.ones((N_CHARS, N_CHARS), dtype=np.float)
    if insert_costs is None:
        insert_costs = np.ones(N_CHARS, dtype=np.float)
    levenshtein_function = pos_aware_levenshtein if pos_aware else levenshtein
    return levenshtein_function(
        text1,
        text2,
        substitute_costs=substitute_costs,
        insert_costs=insert_costs,
        delete_costs=insert_costs
    )


def calc_norm_distance(
    text1: str,
    text2: str,
    substitute_costs: Optional[np.ndarray] = None,
    insert_costs: Optional[np.array] = None,
    pos_aware: bool = False
) -> float:
    """
    Вычисляет расстояние Левенштейна между строками и нормирует на длину
    :param text1: строка, str
    :param text2: строка, str
    :param substitute_costs: веса замен символов, np.ndarray размера 128х128, optional
    :param insert_costs: веса вставок/удалений символов, np.array размера 128, optional
    """
    if text1 or text2:
        return calc_distance(
            text1,
            text2,
            substitute_costs=substitute_costs,
            insert_costs=insert_costs,
            pos_aware=pos_aware
        ) / max(len(text1), len(text2))
    return 0.0


def align_texts(
    ref_words: Sequence[str],
    hyp_words: Sequence[str],
    substitute_cost_func: Callable[[str, str], float] = lambda s, t: 1,
    insert_cost_func: Callable[[str], float] = lambda s: 1,
    stick_cost_func: Optional[Callable[[str, str], float]] = None
) -> Tuple[float, Dict[str, Tuple[int, int]]]:
    """
    Тело динамики пословного Левенштейна для WERP
    :param ref_words: исходный текст 1
    :param hyp_words: исходный текст 2
    :param substitute_cost_func: функция, возвращающая стоимость замены двух слов (передаются в аргументах)
    :param insert_cost_func: функция, возвращающая стоимость вставки слова (передаётся в аргументах)
    :param stick_cost_func: функция, возвращающая стоимость склейки. Если None, то операция склейки не используется
    :return: (cost, operations), где cost — расстояние Левенштейна, operations — словарь операций между текстами (по индексам)

    Пример использования:
        align_texts(
            ['привет',  'братик', 'б'], ['приве', 'а', 'браик'],
            get_pher(s, t, substitute_costs, insert_costs)
        ) == {'SUB': [(0, 0), (1, 2)], 'INS': [(1, 1)], 'DEL': [(2, 3)]}

    """
    ref_len = len(ref_words)
    hyp_len = len(hyp_words)

    costs = [[0] * (hyp_len + 1) for i in range(ref_len + 1)]  # для динамики в Левенштейне
    backtrace = [['MATCH'] * (hyp_len + 1) for i in range(ref_len + 1)]  # тип последней операции

    for i in range(1, ref_len + 1):
        costs[i][0] = costs[i - 1][0] + insert_cost_func(ref_words[i - 1])
        backtrace[i][0] = 'DEL'

    for j in range(1, hyp_len + 1):
        costs[0][j] = costs[0][j - 1] + insert_cost_func(hyp_words[j - 1])
        backtrace[0][j] = 'INS'

    for i in range(1, ref_len + 1):
        for j in range(1, hyp_len + 1):
            if ref_words[i - 1] == hyp_words[j - 1]:
                costs[i][j] = costs[i - 1][j - 1]
                backtrace[i][j] = 'MATCH'
            else:
                sub_cost = costs[i - 1][j - 1] + substitute_cost_func(ref_words[i - 1], hyp_words[j - 1])
                ins_cost = costs[i][j - 1] + insert_cost_func(hyp_words[j - 1])
                del_cost = costs[i - 1][j] + insert_cost_func(ref_words[i - 1])

                stick_ref_cost = INF
                stick_hyp_cost = INF
                if stick_cost_func is not None and i >= 2:
                    stick_ref_cost = costs[i - 2][j - 1] + stick_cost_func(ref_words[i - 2] + ref_words[i - 1], hyp_words[j - 1])
                if stick_cost_func is not None and j >= 2:
                    stick_hyp_cost = costs[i - 1][j - 2] + stick_cost_func(ref_words[i - 1], hyp_words[j - 2] + hyp_words[j - 1])

                costs[i][j] = min(sub_cost, ins_cost, del_cost, stick_ref_cost, stick_hyp_cost)
                if costs[i][j] == sub_cost:
                    backtrace[i][j] = 'SUB'
                elif costs[i][j] == ins_cost:
                    backtrace[i][j] = 'INS'
                elif costs[i][j] == del_cost:
                    backtrace[i][j] = 'DEL'
                elif costs[i][j] == stick_ref_cost:
                    backtrace[i][j] = 'STICK_REF'
                else:
                    backtrace[i][j] = 'STICK_HYP'

    operations = {'SUB': [], 'INS': [], 'DEL': []}
    if stick_cost_func is not None:
        operations['STICK_REF'] = []
        operations['STICK_HYP'] = []

    cur_i = len(ref_words)
    cur_j = len(hyp_words)
    while cur_i > 0 or cur_j > 0:
        op = backtrace[cur_i][cur_j]
        if op == 'MATCH':
            cur_i -= 1
            cur_j -= 1
        elif op == 'SUB':
            operations[op].append((cur_i - 1, cur_j - 1))
            cur_i -= 1
            cur_j -= 1
        elif op == 'INS':
            operations[op].append((cur_i, cur_j - 1))
            cur_j -= 1
        elif op == 'DEL':
            operations[op].append((cur_i - 1, cur_j))
            cur_i -= 1
        elif op == 'STICK_REF':
            operations[op].append((cur_i - 2, cur_j - 1))
            cur_i -= 2
            cur_j -= 1
        elif op == 'STICK_HYP':
            operations[op].append((cur_i - 1, cur_j - 2))
            cur_i -= 1
            cur_j -= 2
    return costs[-1][-1], operations


def check_bad_prefix(word: str, pref: str) -> bool:
    """Проверяет, что слово начинается на данный префикс"""
    return word.startswith(pref)


def is_stop_word(word: str) -> bool:
    """Проверяет, что слово вставлять дорого"""
    for pref in BAD_PREFIX_DIFF:
        if check_bad_prefix(word, pref):
            return True
    return word in STOP_WORDS


def check_bad_prefix_matching(word1: str, word2: str, pref1: str, pref2: str) -> bool:
    """Проверяет, что слова начинаются на данные приставки"""
    if pref1.startswith(pref2) and word1.startswith(pref1) and word2.startswith(pref1):
        return False
    if pref2.startswith(pref1) and word1.startswith(pref2) and word2.startswith(pref2):
        return False
    return word1.startswith(pref1) and word2.startswith(pref2) or \
           word1.startswith(pref2) and word2.startswith(pref1)


def check_bad_prefix_diff(word1: str, word2: str, pref: str, prev_word1: Optional[str] = None, prev_word2: Optional[str] = None) -> bool:
    """
    :param word1: слово 1
    :param word2: слово 2
    :param pref: приставка
    :param prev_word1: слово перед словом 1 (или None)
    :param prev_word2: слово перед словом 2 (или None)
    Проверяет, что одно слово начинается на данную приставку, другое — нет.
    Исключение: если в одном слове приставка отделилась как предлог (например, 'нехорошо' vs 'не хорошо')
    """
    return word1.startswith(pref) and not word2.startswith(pref) and (prev_word2 is None or prev_word2 != pref) or \
           not word1.startswith(pref) and word2.startswith(pref) and (prev_word1 is None or prev_word1 != pref)


def is_bad_sub(word1: str, word2: str, prev_word1: Optional[str] = None, prev_word2: Optional[str] = None) -> bool:
    """
    :param word1: слово 1
    :param word2: слово 2
    :param prev_word1: слово перед словом 1 (или None)
    :param prev_word2: слово перед словом 2 (или None)
    Проверяет, есть ли в словах word1, word2 запретные операции.
    Например, замена приставки 'в' на 'вы', вставка приставки 'не' и другие.
    """
    if (word1, word2) in FINE_WORD_MATCHING or (word2, word1) in FINE_WORD_MATCHING:
        return False
    for pref1, pref2 in BAD_PREFIX_MATCHING:
        if check_bad_prefix_matching(word1, word2, pref1, pref2):
            return True
    for pref in BAD_PREFIX_DIFF:
        if check_bad_prefix_diff(word1, word2, pref, prev_word1, prev_word2):
            return True
    return False


def match_stick(words1: Sequence[str], words2: Sequence[str], i: int, j: int) -> bool:
    return i < len(words1) and j + 1 < len(words2) and words1[i] == words2[j] + words2[j + 1]


def check_for_expensive_operations(text1: str, text2: str,
                                   phonemes1: str, phonemes2: str,
                                   substitute_costs: np.ndarray,
                                   insert_costs: np.array) -> bool:
    """
    Ищет дорогие операции пословного Левенштейна (например, вставка слова 'не' / замена приставки 'в' на 'вы')
    :param text1: текст 1
    :param text2: текст 2
    :param phonemes1: фонемное представление текста 1
    :param phonemes2: фонемное представление текста 2
    :param substitute_costs: веса замен символов, np.ndarray размера N_CHARSxN_CHARS, для get_pher
    :param insert_costs: веса удалений символов, np.array размера N_CHARS, для get_pher
    :return: True, если есть дорогая операция
    """
    words1 = get_words(text1)
    words2 = get_words(text2)
    phonemes1 = get_words(phonemes1)
    phonemes2 = get_words(phonemes2)
    if len(words1) != len(phonemes1) or len(words2) != len(phonemes2):
        # логировать тут, что не проверяли на дорогие вставки-приставки?
        return False

    def _sub_cost(s1: str, s2: str) -> float:
        pher = get_pher(s1, s2, substitute_costs, insert_costs, pos_aware=False)
        if pher >= 0.8:  # 0 значит любые замены запрещены, +inf значит любые замены разрешены
            return INF
        return pher

    _, diff_indices = align_texts(phonemes1, phonemes2, substitute_cost_func=_sub_cost)

    for i, j in diff_indices['INS']:
        if is_stop_word(words2[j]) and not match_stick(words1, words2, i, j):
            # вторая проверка — чтобы на "нехорошо" vs "не хорошо" не срабатывало
            return True

    for i, j in diff_indices['DEL']:
        if is_stop_word(words1[i]) and not match_stick(words2, words1, j, i):
            return True

    for i, j in diff_indices['SUB']:
        prev_word1 = words1[i - 1] if i > 0 else None
        prev_word2 = words2[j - 1] if j > 0 else None
        if is_bad_sub(words1[i], words2[j], prev_word1, prev_word2):
            return True
    return False


# ДЛЯ ПОТЕНЦИАЛЬНОГО РАЗБИЕНИЯ НА МОРФЕМЫ (СЕЙЧАС НЕ ИСПОЛЬЗУЮТСЯ)

class MorphemeParser:
    """
    Наивно (по словарю) разбивает слово на морфемы
    Необработанный словарь есть тут: https://web.archive.org/web/20170328151221/http://speakrus.ru/dict/index.htm#tikhonov
    """
    def __init__(self, dict_path: str = '/tikhonov.txt', russian_prefixes_path: str = '/russian_prefixes.txt'):
        self.word2morph = self._load_word2morph(dict_path)
        self.russian_prefixes = self._load_russian_prefixes(russian_prefixes_path)
        self.morph = MorphAnalyzer()

    @staticmethod
    def _load_word2morph(dict_path: str) -> Dict[str, List[str]]:
        """
        Загружает словарь Тихонова (надо добавить больше несловарных слов)
        :return: словарь {слово -> список его морфем}
        """
        word2morph = {}
        with open(dict_path, 'r', encoding='1251') as file:
            prev_word = ''
            for wordline in file:
                word, meta = wordline.split(' | ')
                word = word.rstrip('12345')
                if word != prev_word:
                    morph = meta.split()[0].rstrip(',')
                    morph = morph.rstrip('12345')
                    morph = morph.replace("'", '').split('/')
                    word2morph[word] = morph
                    prev_word = word
        # дополняем словарь несловарными словами
        word2morph['алиса'] = ['алис', 'а']
        word2morph['алиска'] = ['алис', 'к', 'а']
        word2morph['алисочка'] = ['алис', 'очк', 'а']
        word2morph['алисонька'] = ['алис', 'оньк', 'а']
        return word2morph

    @staticmethod
    def _load_russian_prefixes(russian_prefixes_path: str) -> Set[str]:
        """Загружает русские приставки"""
        prefixes = set()
        with open(russian_prefixes_path, 'r', encoding='utf8') as file:
            prefixes = {line.split() for line in file if line}
        return prefixes

    def get_morphemes_from_word(self, word: str) -> Optional[List[str]]:
        """
        Разбивает слово или её нормальную форму на морфемы
        :param word: слово
        :return: список морфем, если в word2morph присутствует слово или её нормальная форма, None иначе
        """
        # если в слове есть латиница, то слово не русское и на морфемы не разбиваем
        if re.search(r'[a-z]', word):
            return None
        # пробуем разбить на морфемы само слово
        # в самом словаре только инфинитивы, это способ обхода
        # проблем с точностью pymorphy2 и спецслов (алисонька, etc.)
        if word in self.word2morph:
            return self.word2morph[word]
        # пробуем разбить на морфемы нормальную форму слова
        normal_form = self.morph.parse(word)[0].normal_form
        if normal_form in self.word2morph:
            return self.word2morph[normal_form]
        return None

    def get_morphemes(self, text: str) -> List[Optional[List[str]]]:
        """
        Разбиваем слова из текста text на морфемы. Те, что не удалось разбить, оставляет как None.
        :param text: текст
        :return: список списков морфем по словам
        """
        morphemes = []
        for word in re.findall(r'[\w]+', text):
            morphemes_from_word = self.get_morphemes_from_word(word.lower())
            morphemes.append(morphemes_from_word)
            # Может быть, если не разбилось на морфемы (morphemes_from_word is None),
            # лучше не аппендить или не проверять на плохие операции вовсе?
        return morphemes

    def get_word_root(self, word: str) -> Optional[str]:
        """
        Возвращает корень (то есть первую морфему, которая не является приставкой русского языка).
        Проблема: могут быть приставки-корни (воз), приставки-суффиксы (а), корни-суффиксы, двойные корни.
        Это всё никак не обрабатывается.

        :param word: слово
        :return: корень слова, если такой определился, иначе None.
        """
        morphemes = self.get_morphemes_from_word(word)
        if not morphemes:
            return None

        # если только одна морфема, то она корень
        if len(morphemes) - morphemes.count('') == 1:
            return morphemes[0]

        for i, morpheme in enumerate(morphemes):
            if morpheme == '':  # конец слова
                break
            if morpheme not in self.russian_prefixes:
                return morpheme
        # если не получилось найти неприставок, предполагаем, что корень — последняя
        return morphemes[i - 1]
