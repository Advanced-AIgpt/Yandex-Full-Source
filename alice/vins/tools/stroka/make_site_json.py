#!/usr/bin/env python
# coding: utf-8

import json
import codecs
from collections import defaultdict

# TODO:
# 1. DOWNLOAD FROM NIRVANA (nda.ya.ru/3SaRcQ)
# 2. awk -F'\t' '{if ($5=='225' || $5=='-1') print $1"\t"$2}' out_file | \
# sed 's/скачать//g' | sed 's/бесплатно//g' | \
# sed 's/смотреть//g' | sort -u > site.tsv


def make_entities(path_from, path_to):
    js = defaultdict(list)
    with codecs.open(path_from, "r", encoding='utf-8') as f:
        for line in f:
            parts = line.strip().split('\t')
            value, key = parts[0].strip(), parts[1].strip()

            js[key].append(value.lower())

    # убираем повторы
    # synonyms до = ["сайт вконтакте", "вконтакте"]
    # synonyms после = ["вконтакте"]
    for key, synonyms in js.iteritems():
        for item1 in synonyms[:]:
            for item2 in synonyms[:]:
                if item1 != item2:
                    if (' ' + item2 + ' ').find(' ' + item1 + ' ') > -1:
                        if item2 in synonyms:
                            synonyms.remove(item2)
        js[key] = list(set(synonyms))

    with codecs.open(path_to, 'wb', 'utf-8') as f:
        json.dump(js, f, ensure_ascii=False, indent=2)


def main():
    make_entities(
        "site.tsv",
        "site.json"
    )


if __name__ == "__main__":
    main()
