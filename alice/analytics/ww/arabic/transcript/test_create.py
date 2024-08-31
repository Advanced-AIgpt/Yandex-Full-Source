# coding: utf-8
# скрипт создаёт test_input.txt с тестовыми словами

import json
import random


with open('arabic_mapping.json') as f:
    arabic_mapping = json.load(f)


def is_good_word(word):
    has_arabic_chars = any([x in arabic_mapping for x in word])
    return has_arabic_chars


if __name__ == '__main__':
    with open('in1.all_data.json') as f:
        in1 = json.load(f)

    words = ' '.join(in1).split(' ')

    test_words = []

    i = 0
    while i < 2000:
        idx = random.randrange(len(words) - 1)
        word = words[idx]
        if is_good_word(word) and word not in test_words:
            test_words.append(word)
            i += 1

    i = 0
    while i < 1000:
        idx = random.randrange(len(words) - 2)
        word = words[idx] + ' ' + words[idx + 1]
        if is_good_word(word) and word not in test_words:
            test_words.append(word)
            i += 1

    i = 0
    while i < 1000:
        idx = random.randrange(len(words) - 3)
        word = words[idx] + ' ' + words[idx + 1] + ' ' + words[idx + 2]
        if is_good_word(word) and word not in test_words:
            test_words.append(word)
            i += 1

    i = 0
    while i < 500:
        idx = random.randrange(len(words) - 4)
        word = words[idx] + ' ' + words[idx + 1] + ' ' + words[idx + 2] + ' ' + words[idx + 3]
        if is_good_word(word) and word not in test_words:
            test_words.append(word)
            i += 1

    i = 0
    while i < 500:
        idx = random.randrange(len(words) - 5)
        word = words[idx] + ' ' + words[idx + 1] + ' ' + words[idx + 2] + ' ' + words[idx + 3] + ' ' + words[idx + 4]
        if is_good_word(word) and word not in test_words:
            test_words.append(word)
            i += 1

    with open('test_input.txt', 'w') as f:
        for word in test_words:
            f.write(f'{word}\n')

    with open('in1.json', 'w') as f:
        json.dump(test_words, f, indent=4, ensure_ascii=False)
