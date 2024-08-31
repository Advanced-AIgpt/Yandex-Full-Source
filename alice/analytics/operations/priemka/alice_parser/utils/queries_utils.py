# coding: utf-8

import re

EOS_TAG = '<EOS>'
EOSP_TAG = '<EOSp>'


def has_eosp_tag(text):
    """
    Возвращает наличие ASR тега <EOSp> в тексте запроса
    :param Optional[str] text:
    :return bool:
    """
    if text is None:
        return False
    return EOSP_TAG in text


def trim_spaces(string):
    """
    Удаляет лишние пробелы в строке: в начале, в конце; несколько пробелов подряд объединяет в один
    :param str string:
    :return str:
    """
    return re.sub(r' {2,}', ' ', string).strip()


def prepare_query_for_toloka(text):
    """
    Подготавливает текст запроса `text` для отображение в толоке (query)
    :param str text:
    :return str:
    """
    return trim_spaces(
        text.replace(EOS_TAG, ' ').replace(EOSP_TAG, ' ')
    )


def prepare_toloka_queries(query, split_by_eosp_tag=True):
    """
    По запросу пользователя к Алисе, возвращает несколько запросов, которые нужно размечать в толоке
    Учитывает теги разметки ASR
    :param Optional[str] query:
    :param bool split_by_eosp_tag:
    :return Iterator[str]:
    """
    if query is None:
        yield query
        return

    parts = query.split(EOSP_TAG)

    if len(parts) == 1 or split_by_eosp_tag is False:
        # по умолчанию, если <EOSp> тега нет, возвращаем запрос as is
        yield prepare_query_for_toloka(query)
    else:
        yield prepare_query_for_toloka(parts[0])
        yield prepare_query_for_toloka(query)


def clean_voice_text_from_tags(tts_text):
    voice_text = tts_text

    tech_tags = re.compile('|'.join([
        '#ord', '#card',
        '#nom', '#gen', '#dat', '#acc', '#instr', '#loc',
        '#mas', '#fem', '#neu',
        '#sg', '#pl',
        '<\\[[a-zA-Z =\\~\\/\\|]*\\]>',
        '\\.?sil *<\\[ *[0-9]* *\\]>',
        '<speaker[^>]*>',
        '<\\[\\/domain\\]>'
    ]))

    voice_text = tech_tags.sub('', voice_text)
    voice_text = voice_text.replace('.\n\n.sil ', '.\n\n').replace('.sil ', '. ') \
        .replace(' .', '.').replace('  ', ' ').strip()

    # remove '+' before russian vowels
    if '+' in voice_text:
        splitted = voice_text.split('+')
        voice_text = splitted[0]
        for el in splitted[1:]:
            if (el.startswith('а') or el.startswith('е') or el.startswith('ё') or el.startswith('и')
                    or el.startswith('о') or el.startswith('у') or el.startswith('ы') or el.startswith('э')
                    or el.startswith('ю') or el.startswith('я') or el.startswith('А') or el.startswith('Е')
                    or el.startswith('Ё') or el.startswith('И') or el.startswith('О') or el.startswith('У')
                    or el.startswith('Э') or el.startswith('Ю') or el.startswith('Я')):
                voice_text += el
            else:
                voice_text += '+' + el

    return voice_text
