import collections
import json
import os
import re
from string import Template


TOP_K = 50
MAX_N = 3


def calculate_frequency(input_json, n=1):
    """Constructs dictionary of word-frequency mappings (e.g. {'alice': 42})"""
    freq = collections.defaultdict(int)
    for item in input_json:
        text = preprocess(item['text'])
        words = text.split()
        for i in range(len(words) - n + 1):
            ngram = ' '.join(words[i:i + n])
            # TODO(ardulat): take sample weights into account (e.g. freq[ngram] += weight)
            freq[ngram] += 1
    return freq


def preprocess(text):
    text = text.lower()

    text = re.sub(r'[^a-zA-Zа-яА-Я0-9\s]', ' ', text)

    return text


def postprocess(freq):
    top_freq = collections.Counter(freq).most_common(TOP_K)
    # convert {'alice': 42} -> [{'ngram': 'alice', 'frequency': 42}]
    top_freq = [{'ngram': key, 'frequency': value} for key, value in top_freq]
    return top_freq


def construct_ngrams(requests):
    ngrams_dict = collections.defaultdict(dict)
    for i in range(MAX_N):
        ngram_freq = calculate_frequency(requests, n=i+1)
        ngram = postprocess(ngram_freq)
        key = str(i + 1) + '-gram'  # 1-gram, 2-gram, 3-gram
        ngrams_dict[key] = ngram
    return json.dumps(ngrams_dict, ensure_ascii=False)


def render_html_report(ngrams, template_folder='gen_ngram_frequency'):
    with open(os.path.join(template_folder, 'template.html')) as f:
        TEMPLATE = f.read()

    mapping = {
        'FREQUENCY_DATA': json.loads(ngrams)
    }
    return Template(TEMPLATE).substitute(mapping)
