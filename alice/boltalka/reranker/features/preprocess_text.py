# coding=utf-8
import regex as re
import string


class Preprocessor(object):
    def __init__(self):
        self.tokenize_regex = re.compile(u'([' + string.punctuation + ur'\\])', re.UNICODE)
        self.yo_regex = re.compile(u'[ёë]', re.UNICODE)

    def __call__(self, line):
        line = self.tokenize_regex.sub(ur' \1 ', line)
        line = '\t'.join([re.sub('\s+', ' ', turn, flags=re.UNICODE).strip()
                                 for turn in line.split('\t')])
        line = line.lower()
        line = self.yo_regex.sub(u'е', line)
        return line

    def tokenize(self, line):
        line = re.sub('\s+', ' ', line, flags=re.UNICODE).strip()
        return line.split(' ')
