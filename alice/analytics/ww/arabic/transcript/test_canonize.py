# coding: utf-8
# скрипт для файла test_input.txt прогоняет транскриптор и сохраняет результат в test_answer.txt

import json
import random
from arabic_transcript import ArabicTranscripter

if __name__ == '__main__':
    with open('arabic_mapping.json') as f:
        arabic_mapping = json.load(f)

    artr = ArabicTranscripter(arabic_mapping)

    fin = open('test_input.txt')
    fout = open('test_answer.txt', 'w')
    for idx, line in enumerate(fin):
        result = artr.transcript(line.rstrip())
        fout.write(f'{result}\n')

    fin.close()
    fout.close()
