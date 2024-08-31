# coding=utf-8
import re
import string

from liblemmer_python_binding import AnalyzeWord


def lemmatize(word):
    hypothesis = AnalyzeWord(word, langs=['ru'], split=True)
    if len(hypothesis) == 0:
        return word
    return hypothesis[0].Lemma


class Preprocessor(object):
    def __init__(self):
        self.tokenize_regex = re.compile(u'([' + string.punctuation + ur'\\])', re.UNICODE)
        self.yo_regex = re.compile(u'[ёë]', re.UNICODE)

    def __call__(self, line):
        line = line.decode('utf-8')
        line = self.tokenize_regex.sub(ur' \1 ', line)
        line = '\t'.join([re.sub('\s+', ' ', turn, flags=re.UNICODE).strip()
                              for turn in line.split('\t')])
        line = line.lower()
        line = self.yo_regex.sub(u'е', line)
        return line


def tokenize_words(line):
    return line.split(' ')


def tokenize_trigrams(line):
    line = ' ' + line + ' '
    return [line[i:i+3] for i in xrange(len(line)-2)]


def tokenize_lemmas(line):
    words = tokenize_words(line)
    return [lemmatize(word) for word in words]

