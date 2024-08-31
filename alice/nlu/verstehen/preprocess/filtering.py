import string
from collections import OrderedDict
from functools import partial

from verstehen.util import mp_map
from .preprocessing import lemmatize_token


def _filter_duplicate_texts(texts, text_preprocessing_fn=None, processes=1):
    """
    Filtering duplicate texts with an optional preprocessing prior to string duplcate comparison.
    If two strings considered the same, then only its first occurrence remains in the final list.

    Arguments:
        texts: list of strings to filter
        text_preprocessing_fn: optional function that is used for each element from `texts` in order
            to compare strings for duplication. Should be pickleable if `processes > 1`.
        processes: number of processes in parallel used to preprocess texts.
    """
    original_texts = texts
    if text_preprocessing_fn is not None:
        texts = mp_map(text_preprocessing_fn, texts, processes=processes)

    included_texts = set()
    included_ids = []

    for i, text in enumerate(texts):
        if text not in included_texts:
            included_ids.append(i)
            included_texts.add(text)

    filtered_texts = [original_texts[id] for id in included_ids]
    return filtered_texts


def lemmatized_tokens_string_transform(text, order_sensitive=False, duplicate_tokens_sensitive=False):
    """
    Function that transforms text into a string of lemmatized tokens with the ability
    to take into account order of tokens and their duplicates.

    Arguments:
        text: string of tokens separated by space character
        order_sensitive: bool flag whether the transformation should output distinct results
            for different order of tokens in the original `text` string.
        duplicate_tokens_sensitive: bool flag whether the result should output distinct results
            for `text` string containing same tokens more than once.
    """
    tokenized = [lemmatize_token(token) for token in text.split()]
    if not duplicate_tokens_sensitive:
        tokenized = list(OrderedDict.fromkeys(tokenized))
    if not order_sensitive:
        tokenized = sorted(tokenized)
    return ' '.join(tokenized)


def filter_duplicates_by_lemmatized(texts, order_sensitive=False, duplicate_tokens_sensitive=False, processes=1):
    """
    Filtering list of texts for duplicates by comparison of their lemmatized versions.

    Arguments:
        texts: list of strings to filter
        order_sensitive: bool flag whether order of tokens in elements of `texts` will be considered
            when comparing strings.
        duplicate_tokens_sensitive: bool flag whether duplication of tokens in elements of `texts`
            will be considered when comparing strings.
        processes: number of processes in parallel used to preprocess texts.
    """
    preprocessing_fn = partial(
        lemmatized_tokens_string_transform,
        order_sensitive=order_sensitive,
        duplicate_tokens_sensitive=duplicate_tokens_sensitive
    )
    return _filter_duplicate_texts(texts, text_preprocessing_fn=preprocessing_fn, processes=processes)


def filter_duplicates_by_lower_case(texts, processes=1):
    """
    Filtering list of texts for duplicates by comparison of their case-insensitive versions.

    Arguments:
        texts: list of strings to filter
        processes: number of processes in parallel used to preprocess texts.
    """
    return _filter_duplicate_texts(texts, text_preprocessing_fn=string.lower, processes=processes)
