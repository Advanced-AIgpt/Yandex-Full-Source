import re
import unicodedata
from collections import defaultdict, Counter

from .normalize import (
    patch_alef_maksura,
    patch_ta_marbuta,
    normalize_text,
)
from .arabic_const import SPOKEN_NOISE, EMPTY_WORD

CONFIG = {
    # количество слов, разница между количеством слов в самом частотном тексте,
    # по сравнению с остальными текстами, при которой считаем, что запрос плохо проаннотирован
    'WORDS_COUNT_BAD_DIFF': 4,

    # сколько должно быть мимально запросов с одинаковым числом слов, чтобы можно было применять пословную аггрегацию
    'SAME_WORDS_COUNT_FOR_WORDS_MV': 2,

    # максимальное количество несогласованных слов в результате, чтобы мы считали результат уверенным
    'BAD_WORDS_THRESHOLD': 0,

    # какое количество запросов из 5ти должно совпасть, чтобы сразу считалось Majority Vote по полному тексту
    'MIN_SAME_TEXTS_FOR_FULL_MV': 3,

    # отключить первичную аггрегацию через MV, т.е. всегда пробовать сначала аггрегировать по словам
    'DISABLE_FULL_MV': False,

    # отключить аггрегацию по словам, т.е. всегда аггрегация по полному тексту аннотации
    'DISABLE_WORDS_MV': False,
}


def aggregate_item_mv_full_impl(texts):
    """
    Аггрегирование MV

    :param Dict[str, int] texts: Counter() с текстами: ключом является текст, значением - количество
    :return Tuple[str, int, int]: Самый частотный текст, число голосов за него, число голосов всего
    """
    if not texts:
        return SPOKEN_NOISE, 0, 0

    votes = Counter()
    total_votes = 0
    for text, cnt in texts.items():
        votes[text] += cnt
        total_votes += cnt

    # сортировка результатов:
    # берём самый частотный текст без ?
    # при одинаковой частоте - самый длинный (~=информативный) текст
    result = sorted(
        votes.most_common(),
        key=lambda x: (SPOKEN_NOISE in x[0], -x[1], -len(x[0])),
    )

    return (
        result[0][0],
        result[0][1],
        total_votes
    )


def aggregate_item_mv_one_word_impl(texts):
    """
    Пословная аггрегация для текстов `texts`

    :param List[List[str, int]] texts: массив из массивов по каждому слову: [сам само слово, число слов во всём запросе]
    :return Tuple[str, int, int, int]: выбранное слово, число голосов за него, число голосов всего, число голосов за топ-2 слово
    """
    if not texts:
        return SPOKEN_NOISE, 0, 0, 0

    votes = Counter()
    total_votes = 0
    full_texts_words_count = {}
    for item in texts:
        votes[item[0]] += 1
        total_votes += 1

        # считаем количество слов в каждом запросе для каждого соответсвующего слова чтобы учитывать как вес при вторичной сортировке
        if item[0] != SPOKEN_NOISE:
            full_texts_words_count[item[0]] = max(
                full_texts_words_count.get(item[0], 0),
                item[1]
            )

    result = sorted(
        votes.most_common(),
        # выбор слова, ранжирование:
        key=lambda x: (
            SPOKEN_NOISE in x[0],  # не должно быть SPN
            -x[1],  # первичная сортировка: по частоте встречаемости - самое частотное
            -full_texts_words_count[x[0]],  # вторичная сортировка - из самого длинного предложения
            -len(x[0])  # третичная: самое длинное слово
        ),
    )

    return (
        result[0][0],
        result[0][1],
        total_votes,
        result[1][1] if result and len(result) > 1 and result[1][0] != EMPTY_WORD else 0  # число голосов за вторую гипотезу и она не null
    )


def get_words_list(texts, words_count):
    """
    Формирует массив массивов со словами из всех запросов

    :param Dict[str, int] texts: Counter() с текстами: ключом является текст, значением - количество
    :param int words_count: число слов в самом часто встречаемом тексте
    :return List[List[str]]:
    """
    words_list = []
    for text, cnt in texts.items():
        # формируем массив с массивами слов
        words = text.split(' ')
        if len(words) == words_count:
            words_list += [words] * cnt
        else:
            # TODO: что-то сделать с текстами с различным числом слов, сейчас — они просто считаются в MV as is и это неплохо работает
            # можно сделать alignment https://biopython.org/docs/1.75/api/Bio.pairwise2.html
            # или, например, для выравнивания нескольких слов, можно уменьшать суммарный LD
            words_list += [words] * cnt
    return words_list


def aggregate_item_words_impl(texts, words_count):
    """
    Основной код функции пословной аггрегации

    :param Dict[str, int] texts: Counter() с текстами: ключом является текст, значением - количество
    :param int words_count: число слов в самом часто встречаемом тексте
    :return Tuple[str, int, int, List[Dict]]: выбранное слово, число голосов за него, число голосов всего, статистика по каждому слову
    """
    sum_mv_votes, sum_total_votes = 0, 0

    words_list = get_words_list(texts, words_count)

    result_words = []
    result_words_votes = []
    for i in range(words_count):
        # формируем массив из [слово; длина всего запроса], чтобы выбирать из них наилучшее
        current_word_list = []
        for word_arr in words_list:
            if i >= len(word_arr) or word_arr[i] == SPOKEN_NOISE or word_arr[i] == EMPTY_WORD:
                continue
            current_word_list.append([
                word_arr[i],  # само слово
                len([x for x in word_arr if x != SPOKEN_NOISE])  # количество слов в запросе
            ])
        current_word_list = patch_ta_marbuta(current_word_list)
        current_word_list = patch_alef_maksura(current_word_list)

        result, mv_votes, total_votes, second_mv_votes = aggregate_item_mv_one_word_impl(current_word_list)
        result_words.append(result)
        sum_mv_votes += mv_votes
        sum_total_votes += total_votes
        result_words_votes.append({
            'mv': mv_votes,
            'second': second_mv_votes,
            'total': total_votes,
            'word': result,
        })

        # вопросики убираем из MV, но учитываем в числе total голосов
        spns = [x[i] for x in words_list if len(x) > i and x[i] == SPOKEN_NOISE]
        sum_total_votes += len(spns)

    return ' '.join(result_words), sum_mv_votes, sum_total_votes, result_words_votes


def get_words_count(texts):
    """
    Возвращает "самое часто встречающееся" количество слов

    :param Dict[str, int] texts: Counter() с текстами: ключом является текст, значением - количество
    :return Tuple[int, int]: количество слов, количество запросов с таким числом слов
    """
    words_count_list = []
    for text, cnt in texts.items():
        if not text or text == EMPTY_WORD:
            continue
        length = len(text.split(' '))
        words_count_list += [length] * cnt

    if len(words_count_list) == 0:
        # кейс с {'null': 10}
        return 0, 0

    # считаем длину слов по самому 1) частотному 2) длинному предложению
    most_common = sorted(Counter(words_count_list).most_common(), key=lambda x: (-x[1], -x[0]))

    avg_words_count = sum(words_count_list) / len(words_count_list)
    if abs(avg_words_count - most_common[0][0]) > CONFIG['WORDS_COUNT_BAD_DIFF']:
        # если длина самого частотного слова сильно отличается от среднего типа [(3, 2), (12, 1), (9, 1), (7, 1)]
        return 0, 0

    return most_common[0]


def adapt_mv_full_results(result_mv, votes_mv, votes_total):
    """
    Адаптирует формат ответа позапросной MV аггрегации к формату пословной аггрегации
    По-сути умножает число голосов на количество слов в запросе

    :param str result_mv:
    :param int votes_mv:
    :param int votes_total:
    :return Tuple[str, int, int, None]:
    """
    words_count_mv = len(result_mv.split(' '))
    return (
        result_mv,
        votes_mv * words_count_mv,
        votes_total * words_count_mv,
        None
    )


def aggregate_item(texts):
    """
    Выполняет аггрегацию текстов
    Определяет как выполнять аггрегацию: позапросный MV или пословный MV

    :param Dict[str, int] texts: _description_
    :param bool no_mv_words: принудительно выключает пословную аггрегацию
    :return Tuple[str, int, int, Optional[List[Dict]]]:
    """
    (result_mv, votes_mv, votes_total) = list(aggregate_item_mv_full_impl(texts))

    # если по обычному MV совпали 3+ запроса — то берём их
    if result_mv != EMPTY_WORD and votes_mv >= CONFIG['MIN_SAME_TEXTS_FOR_FULL_MV'] and not CONFIG['DISABLE_FULL_MV']:
        return adapt_mv_full_results(result_mv, votes_mv, votes_total)

    words_count, max_votes = get_words_count(texts)

    if max_votes >= CONFIG['SAME_WORDS_COUNT_FOR_WORDS_MV'] and not CONFIG['DISABLE_WORDS_MV']:
        return aggregate_item_words_impl(texts, words_count)
    else:
        # если не получается сделать пословную аггрегацию — то возвращаем как есть
        return adapt_mv_full_results(result_mv, votes_mv, votes_total)


def get_any_unwanted_chars(text):
    """
    Возвращает "плохие" символы из строки `text`
    Разрешён Арабский, Английский, пробел, ? (SPN)

    TODO: сравнить с диапазоном и согласовать с кем:moath-alali: https://a.yandex-team.ru/svn/trunk/arcadia/voicetech/asr/tools/arabic/normalizer/normalizer.py?rev=r9296414#L44

    :param str text: текст запроса
    :return str: пустая строка, в случае, если все символы ОК. Иначе эти символы
    """
    text = re.sub('[ a-z\?]+', '', text)  # noqa
    text = ''.join([
        ch
        for ch in text
        if not (
            'arabic' in unicodedata.name(ch, '').lower()
            or 'persian' in unicodedata.name(ch, '').lower()
        )
    ])

    return text


def is_bad_text(text):
    """
    Возвращает True, если текст в результате получился плохой/неуверенный

    :param str text: _description_
    :return bool: _description_
    """
    if text is None:
        return True
    if text == EMPTY_WORD:
        return True
    if SPOKEN_NOISE in text:
        return True
    if get_any_unwanted_chars(text):
        return True
    return False


def is_good_votes_for_word(votes):
    """
    Определяет, что слово получилось уверенное
    Чтобы слово считалось уверенным, за него должны проголосовать хотябы 2 человека

    Считаем ещё приемлемыми варианты: 2,1,4; 2,1,5; 2,2,5; 4,4,10

    :param Dict votes: объект с голосами за слова: mv, second, total
    :return bool:
    """
    if votes['mv'] == 1:
        # все варианты разные: 1,1,1
        return False
    return votes['mv'] >= votes['second']


def is_bad_item(text, mv_votes, words_votes):
    """
    Логика определения, насколько уверенный/неуверенный результат получился

    :param str text: _description_
    :param int mv_votes: _description_
    :param Dict words_votes: _description_
    :param int words_count: число слов в `text`
    :return bool:
    """
    # если в результате остался ? (SPN)
    if is_bad_text(text):
        return True

    words_count = len(text.split(' '))

    # пословная аггрегация
    if words_votes is not None:
        cnt_bad_words = len([x for x in words_votes if not is_good_votes_for_word(x)])
        bad_words_ratio = cnt_bad_words / words_count

        if bad_words_ratio > CONFIG['BAD_WORDS_THRESHOLD']:
            # для пословных аггрегаций считаем запрос плохим, если хотябы 1 слово не уверенное
            return True
    else:
        # про позапросной аггрегации:
        # все гипотезы совсем разные (т.е. MV_VOTES == 1)
        if mv_votes < words_count * 3:
            return True

    return False


def process_item(answers_raw_dict, normalizers='<EMPTY>', config_override=None):
    """
    Аггрегация гипотез аннотирования
    Выполняет позапросную или пословную аггрегацию
    Возвращает json объект с результатами и статистикой
    Также определяет уверенность ответа

    :param Dict[str, int] answers_raw_dict: Counter() с текстами: ключом является текст, значением - количество
    :param str|List normalizers: нормализаторы, которые применять к тексту. По-умолчанию, при <EMPTY> - используются все существующие нормализаторы
    :param Dict[str, int] config_override: Dict с возможностью переопределить константы снаружи
    :return Dict:
    """

    if config_override:
        global CONFIG
        CONFIG.update(config_override)

    # нормализация текста
    answers_normalized = defaultdict(int)
    for text, cnt in answers_raw_dict.items():
        if text is not None:
            text = normalize_text(text, normalizers)
        answers_normalized[text] += cnt

    # непосредственно аггрегация
    result, mv_votes, total_votes, words = aggregate_item(answers_normalized)

    return {
        'result_mv': result,
        'is_bad_item': is_bad_item(result, mv_votes, words),
        'mv_probability': mv_votes / total_votes,
        'stats': {
            'mv_words_votes': mv_votes,
            'total_words_votes': total_votes,
            'words_count': len(result.split(' ')),
            'words_details': words,
        },
        'answers_normalized': answers_normalized,
    }
