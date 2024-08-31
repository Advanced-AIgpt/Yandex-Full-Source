# coding=utf-8
import re
import string

from yandex_lemmer import AnalyzeWord


def lemmatize_token(token):
    infos = AnalyzeWord(token, split=False, langs=['ru'])
    return infos[0].Lemma if len(infos) > 0 else token


def replace_characters_with_pad_and_trim(text, pad=' ', characters=string.punctuation + '\t\n'):
    if isinstance(text, unicode):
        translate_map = dict((ord(c), unicode(pad)) for c in characters)
        text = text.translate(translate_map)
    elif len(pad) == 1:
        translate_map = string.maketrans(characters, pad * len(characters))
        text = text.translate(translate_map)
    return ' '.join(text.split())


def alice_dssm_applier_preprocessing(text):
    """
    Preprocessing that mimics behaviour for DSSM models on Nirvana.
    Based on:
    https://a.yandex-team.ru/arc/trunk/arcadia/alice/boltalka/libs/text_utils/utterance_transform.h?rev=4674509#L63
    """

    # 1. To lower case
    text = text.lower()

    # 2. Surrounding Python punctuation characters with spaces
    text = re.sub(r'([{}])'.format(re.escape(string.punctuation)), r' \1 ', text)

    # 3. Trimming the text and reducing consequent spaces to one space
    text = ' '.join(text.split())

    # 4. Removing first Alice utterances from the beginning and ending of the text
    alices = (u'алиса', u'алис')

    for prefix in alices:
        if text.startswith(prefix + ' '):
            text = text[len(prefix) + 1:]
            break

    for suffix in alices:
        if text.endswith(' ' + suffix):
            text = text[:-(len(suffix) + 1)]
            break

    return text


def text_to_sequence(text, split=' ', filters=string.punctuation + '\t\n'):
    """
    Simplified version of keras_preprocessing.text.text_to_word_sequence
    """
    text = replace_characters_with_pad_and_trim(text, pad=split, characters=filters)
    return text.split(split)


def get_text_preprocessing_fn(function_name):
    text_preprocessing_functions = {
        'alice_dssm_applier_preprocessing': alice_dssm_applier_preprocessing,
        'replace_characters_with_pad_and_trim': replace_characters_with_pad_and_trim
    }

    if function_name not in text_preprocessing_functions:
        raise ValueError('Cannot find function name `{}` in the list of available functions: {}'.format(
            function_name, text_preprocessing_functions.keys()
        ))

    return text_preprocessing_functions[function_name]
