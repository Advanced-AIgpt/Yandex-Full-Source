import re
import string


def punct_separate(text):
    # space out punctuations
    for c in string.punctuation:
        if c in '[]<>_':
            continue

        text = text.replace(c, ' {} '.format(c))
    return text


def preprocess_contexts(contexts):  # TODO compare with prod and remove from data VH making
    # removing \n and \t from contexts
    contexts = [ctx.replace('\n', '').replace('\t', '') for ctx in contexts]

    # ё -> е
    contexts = [ctx.replace('ё', 'е') for ctx in contexts]

    # removing <speaker_voice='aa'>
    contexts = [re.sub('<.*>', ' ', ctx) for ctx in contexts]

    # space out punctuations
    contexts = [punct_separate(ctx) for ctx in contexts]

    # TODO remove the "-" removal when it is fixed in production
    contexts = [ctx.replace(' - ', ' ') for ctx in contexts]

    # TODO remove filtering empty contexts to preserve deterministic length of the array
    # filtering empty contexts
    contexts = [ctx for ctx in contexts if len(ctx) > 0]

    # lowering
    contexts = [ctx.lower() for ctx in contexts]

    return contexts


def preprocess_text_and_tokenize(text, tokenizer, skip_preprocessing=False):
    special_token_pattern = '(\\[[a-zA-Z\d_]+\\])'

    preprocessed_and_tokenized_text_parts = []
    # each text is split by special tokens to subparts which are preprocessed and then joined in the same order
    # with the same order
    for text_part in re.split(special_token_pattern, text):
        # if it is not a special token, we preprocess it and tokenize it
        if not re.match(special_token_pattern, text_part):
            if not skip_preprocessing:
                preprocessed = preprocess_contexts([text_part])
                text_part = preprocessed[0] if len(preprocessed) > 0 else ''
            text_part = tokenizer.tokenize(text_part.encode('utf-8')).decode('utf-8')

        preprocessed_and_tokenized_text_parts.append(text_part)

    return ' '.join(preprocessed_and_tokenized_text_parts)


def preprocess_contexts_and_tokenize(contexts, separator_token, tokenizer, skip_preprocessing=False):
    preprocessed_contexts = [
        preprocess_text_and_tokenize(ctx, tokenizer=tokenizer, skip_preprocessing=skip_preprocessing)
        for ctx in contexts
    ]

    if separator_token and len(separator_token) != 0:
        padded_separator = ' {} '.format(separator_token)
        return padded_separator.join(preprocessed_contexts)
    else:
        return ' '.join(preprocessed_contexts)


# manual declaration since tokenizer doesn't work in Py3
HYPHEN_PREFIXES = {
    'кто', 'кого', 'кому', 'кем', 'ком',
    'что', 'чего', 'чему', 'чем',
    'где',
    'когда',
    'куда',
    'откуда',
    'почему',
    'зачем',
    'как',
    'сколько', 'скольких', 'скольким', 'сколькими',
    'какой', 'какого', 'какому', 'каким', 'каком', 'какие', 'каких', 'какими',
    'чей', 'чьего', 'чьему', 'чьим', 'чьем', 'чьи', 'чьими', 'чьих'
}
HYPHEN_POSTFIXES = {'то', 'нибудь', 'либо'}


def should_insert_hyphen(word1, word2):
    return word1 == 'кое' and word2 in HYPHEN_PREFIXES or word1 in HYPHEN_PREFIXES and word2 in HYPHEN_POSTFIXES


def insert_hyphens(text):
    result_string = []

    prev_token = None
    for token in text.split(' '):
        if prev_token is not None:
            if should_insert_hyphen(prev_token, token):
                result_string.append('-')
            else:
                result_string.append(' ')

        result_string.append(token)
        prev_token = token

    return ''.join(result_string)


def process_quotes(text):
    for quote_character in ['\'', '"']:
        text = re.sub(
            '({}\s+)([^{}].+)(\s+{})'.format(quote_character, quote_character, quote_character),
            '{}\\2{}'.format(quote_character, quote_character),
            text
        )
    return text


def process_response(text, capitalize=False):
    text = text.replace(' `', '')

    for c in string.punctuation:
        if c in '"\'()':
            continue

        text = text.replace(' {} '.format(c), '{} '.format(c))
        text = text.replace(' {}'.format(c), '{}'.format(c))

    text = process_quotes(text)

    # TODO remove the "-" addition when it is fixed in production
    text = insert_hyphens(text)

    if capitalize:
        capitalized = []
        for segment in text.split('.'):
            capitalized.append(segment.lstrip().capitalize())
        text = '. '.join(capitalized)

    return text
