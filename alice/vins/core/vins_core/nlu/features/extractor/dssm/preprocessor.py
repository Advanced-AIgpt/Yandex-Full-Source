import os
import re
import string

from vins_core.nlu.features.extractor.dssm.utils import BOS, EOS, UNK
from vins_core.nlu.features.extractor.dssm.utils import load_dictionary


class TextPreprocessor(object):
    def __init__(self, config, dicts_path):
        self.config = config
        self._dicts_path = dicts_path

        punctuation = ''.join(
            [p for p in string.punctuation if p not in set(self.config.keep_marks)]
        )
        self.punctuation_regex = re.compile(u'[' + punctuation + ']', re.UNICODE)
        self.spaces_regex = re.compile(r'\s+', re.UNICODE)

        self.line_converter = config.line_converter

        self._init_dict()
        self.converter_params = {'word_dct': self.word_dct,
                                 'trigram_dct': self.trigram_dct,
                                 'wbigram_dct': self.wbigram_dct}

    def _get_dict_path(self, dict_name):
        return os.path.join(self._dicts_path, dict_name)

    def _init_dict(self):
        additional = []

        self.word_dct, self.word_dct_size = load_dictionary(
            self._get_dict_path(self.config.word_dct),
            [BOS, EOS, UNK] + additional,
            self.config.word_dct_size,
            self._get_dict_path(
                self.config.keep_word_indices
            ) if self.config.keep_word_indices else None
        )
        self.trigram_dct, self.trigram_dct_size = load_dictionary(
            self._get_dict_path(self.config.trigram_dct),
            additional,
            self.config.trigram_dct_size
        )
        if self.config.wbigram_dct is not None:
            self.wbigram_dct, self.wbigram_dct_size = load_dictionary(
                self._get_dict_path(self.config.wbigram_dct),
                [],
                self.config.wbigram_dct_size
            )
        else:
            self.wbigram_dct, self.wbigram_dct_size = None, None

    def parse_context(self, context_parts):
        'Gets outer and inner context as lists, returns line_ids'
        if self.config.encode_inner_context:
            context_parts.append("")
        line_ids = {
            "text_%s" % i: self.apply_converter(part) for i, part in enumerate(context_parts)
        }
        return line_ids

    def normalize_string(self, str_in):
        str_in = str_in.lower()
        if self.config.strip_punctuation:
            str_in = self.punctuation_regex.sub(' ', str_in)
        str_in = self.spaces_regex.sub(' ', str_in).strip()
        return str_in

    def apply_converter(self, str_in):
        "Gets string from context or whole reply, returns line_ids"
        str_in = self.normalize_string(str_in)

        return self.line_converter(str_in, **self.converter_params)
