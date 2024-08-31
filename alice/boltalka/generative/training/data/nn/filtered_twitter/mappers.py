import re

from yandex_lemmer import AnalyzeWord


class ContextFilterMapper:
    def __init__(self, columns, truncate_non_empty_len=None):
        self.columns = columns
        self.truncate_non_empty_len = truncate_non_empty_len

    def __call__(self, row):
        found_violation = False
        for column in self.columns:
            value = row[column]

            if value == '':
                continue

            if found_violation or self.to_filter(value):
                row[column] = ''
                found_violation = True

        if self.truncate_non_empty_len is None or \
                sum([row[column] != '' for column in self.columns]) >= self.truncate_non_empty_len:
            yield row

    def to_filter(self, value):
        raise NotImplementedError()


class FilterContextsByDicts(ContextFilterMapper):
    def __init__(self, columns, truncate_non_empty_len=None, dict_filepaths=[], whitelist_filtering=True,
                 lemmatize=False, preprocessing_regex=None):
        '''
        :param dict_filepaths: paths to files of dicts where each line is word.
        :param whitelist_filtering: True - leave contexts where all words are in dicts,
                                    False - leave contexts where no words in the context are in dict.
        :param lemmatize: whether to lemmatize words before looking up in a dict.
        :param preprocessing_regex: if str then all contexts are preprocessed with this regex and replaced by
                                    an empty string.
        '''

        super().__init__(columns, truncate_non_empty_len)
        self.dict_filepaths = dict_filepaths
        self.dict_words = None
        self.lemmatize = lemmatize
        self.whitelist_filtering = whitelist_filtering
        self.preprocessing_regex = preprocessing_regex

    def start(self):
        self.dict_words = set()
        for path in self.dict_filepaths:
            with open(path) as f:
                for line in f.readlines():
                    self.dict_words.add(line.rstrip('\n'))

    def lemmatize_token(self, token):
        infos = AnalyzeWord(token, split=False, langs=['ru'])
        return infos[0].Lemma if len(infos) > 0 else token

    def to_filter(self, value):
        if self.preprocessing_regex is not None:
            value = re.sub(self.preprocessing_regex, '', value)

        value = re.sub(r'\s+', ' ', value).strip()

        words = value.split()
        if self.lemmatize:
            words = [self.lemmatize_token(word) for word in words]

        word_in_dict = [word in self.dict_words for word in words]

        if self.whitelist_filtering:
            return not all(word_in_dict)
        else:
            return any(word_in_dict)


class FilterContextsByTokenNumber(ContextFilterMapper):
    def __init__(self, columns, truncate_non_empty_len=None, min_n_tokens=None):
        super().__init__(columns, truncate_non_empty_len)
        self.min_n_tokens = min_n_tokens

    def to_filter(self, value):
        return len(value.split()) < self.min_n_tokens
