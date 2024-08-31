# coding: utf-8
# скрипт для файла test_input.txt прогоняет транскриптор и сохраняет результат в test_answer.txt

import json
import random
from arabic_transcript import ArabicTranscripter

if __name__ == '__main__':
    with open('arabic_mapping.json') as f:
        arabic_mapping = json.load(f)

    artr = ArabicTranscripter(arabic_mapping)

    finput = open('test_input.txt')
    fanswer = open('test_answer.txt')

    passed, total = 0, 0
    for (arabic_utterance, answer) in zip(finput, fanswer):
        arabic_utterance = arabic_utterance.rstrip()
        answer = answer.rstrip()

        result = artr.transcript(arabic_utterance)
        if result == answer:
            passed += 1
        else:
            space = ' ' * 22
            arabic_codes = [ord(x) for x in arabic_utterance]
            arabic_names = [artr.ar_name(x) for x in arabic_utterance]
            print(f'[x] {total:4d} TEST ERROR. CANONIZED: "{answer}"   ARABIC: "{arabic_utterance}"   {arabic_codes} \n {space} RESULT: "{result}"    {arabic_names}')

        total += 1
        # if total >= 200:
        #     break

    finput.close()
    fanswer.close()

    print(f'TESTS PASSED: {passed}, ERRORS: {total-passed}, TOTAL: {total}')
