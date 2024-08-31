# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import logging
import re

from alice.nlu.py_libs.utils import sample as sample_m


logger = logging.getLogger(__name__)
# only alphabetic
_ALPHA = r'[^\W\d_]+'
# alphabetic + numbers
_ALPHANUM = r'[^\W_]+'
# multi-words with separator (multi-word, multi/word, etc.)
# partial words contain only alphabetic symbols (any 1-word or word-1 will be ignored)
_COMPOSED_WORD = r'{0}(?:[\-/\\]{0})+'.format(_ALPHA)
# any number (int, float)
_NUMBER = r'(?:\d+[,\.]\d+|\d+)'
# negative number -1 only at the start of the token
_SIGNED_NUMBER = r'^\-{0}'.format(_NUMBER)
# datetime of type dd.mm.yy{yy}
_DATE = r'\d{1,2}[\.,\\/\-]\d{1,2}[\.,\\/\-]\d{2,4}'
# time of type hh:mm
_TIME = r'\d{1,2}:\d\d'
# arithmetic operations, if inside tokens, should be surrounded by digits
_ARITHMETIC = r'(?:^|(?<=\d))[\+\-/\*=\^](?:$|(?=\d))'
# currency symbols
_CURRENCY = r'[\$\u20ac\u20bd]'
# special cases with glued punctuation
_GLUED_PUNCT = r'\w\+\+'


class FstNormalizerError(ValueError):
    pass


class SampleNormalizer(object):
    """Processor which normalizes input sample. Session need not to be provided in ``__call__`` method.
    """
    _TOKEN = r'({0})'.format('|'.join((
        _GLUED_PUNCT, _COMPOSED_WORD, _DATE, _TIME, _SIGNED_NUMBER,
        _ARITHMETIC, _NUMBER, _ALPHANUM, _CURRENCY,
    )))

    def __init__(self, fst_normalizer):
        self._fst_normalizer = fst_normalizer

    @property
    def is_normalizing(self):
        return True

    @classmethod
    def _extract_tokens(cls, raw_token):
        return re.findall(cls._TOKEN, raw_token, flags=re.U)

    def _pre_normalization(self, sample):
        n_toks, n_tags = [], []

        for tok, tag in zip(sample.tokens, sample.tags):
            # normalizer bug: replace 'ε' symbol (special symbol in OpenFST)
            # check test_fakes_101
            tok = tok.replace('ε', 'e')
            # separating words glued together by comma
            toks = self._extract_tokens(tok)
            # remove singleton punctuation
            if not toks:
                continue
            n_toks.extend(toks)
            following_tag = tag
            if following_tag.startswith('B-'):
                following_tag = 'I-' + following_tag[2:]
            n_tags.extend([tag] + [following_tag] * (len(toks) - 1))
        return sample_m.Sample.from_string(''.join(n_toks), tokens=n_toks, tags=n_tags)

    def _run_fst_normalizer(self, text):
        n_text = self._fst_normalizer(text).lower().strip()
        # fix normalizer pattern:
        # leading zeros become space-delimited after denormalization
        zeros = re.search('((?: |^)0(?: 0)+)', n_text)
        if zeros:
            zstr = zeros.group(1)
            n_text = n_text.replace(zstr, ' ' + re.sub(r'\s+', '', zstr))
        return n_text

    def _normalization(self, sample):
        out_toks, out_tags = [], []
        for toks, indices, tags, segment_tag in sample.group_by_slot():
            n_toks = self._run_fst_normalizer(' '.join(toks)).split()
            n_tags = [segment_tag] * len(n_toks)
            if segment_tag != 'O':
                # sequence of tags usually starts with B-tag, but can start with I-, if it is a continued slot
                first_prefix = tags[0][:2]
                n_tags[0] = first_prefix + n_tags[0]
                for i in xrange(1, len(n_tags)):
                    n_tags[i] = 'I-' + n_tags[i]
            out_toks.extend(n_toks)
            out_tags.extend(n_tags)

        self._validate_normalization(sample.tokens, out_toks)

        return sample_m.Sample.from_string(''.join(out_toks), tokens=out_toks, tags=out_tags)

    def _validate_normalization(self, input_tokens, output_tokens):
        normalized_tokens = self._run_fst_normalizer(' '.join(input_tokens)).split()
        if normalized_tokens != output_tokens:
            raise FstNormalizerError(
                'Tokens might be unaligned with tags for sample:\n'
                'Fully normalized utterance: "%s"\n'
                'Partially normalized utterance: "%s"\n'
                'Initial input: "%s"',
                ' '.join(normalized_tokens),
                ' '.join(output_tokens),
                ' '.join(input_tokens)
            )

    @classmethod
    def _post_normalization(cls, sample):
        n_toks = []
        for tok in sample.tokens:
            # remove redundant whitespaces if any
            tok = re.sub(r'\s+', ' ', tok)
            # ё normalization
            tok = re.sub('ё', 'е', tok, re.UNICODE | re.IGNORECASE)
            n_toks.append(tok)
        return sample_m.Sample.from_string(''.join(n_toks), tokens=n_toks, tags=sample.tags)

    def normalize(self, sample, *args, **kwargs):
        sample = self._pre_normalization(sample)
        sample = self._normalization(sample)
        sample = self._post_normalization(sample)
        return sample
