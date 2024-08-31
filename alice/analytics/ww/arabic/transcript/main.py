# coding: utf-8


import json
import logging

from arabic_transcript import ArabicTranscripter
from arabic_buckwalter import arabic_to_buckwalter
from arabic_phonetise import phonetise


if __name__ == '__main__':
    # включить логирование
    logging.basicConfig(
        level=logging.INFO,
        format="[%(asctime)s] %(levelname)s [%(name)s.%(funcName)s:%(lineno)d] %(message)s",
    )

    with open('in1.json') as f:
        in1 = json.load(f)

    with open('arabic_mapping.json') as f:
        arabic_mapping = json.load(f)
    artr = ArabicTranscripter(arabic_mapping)

    phonemes_with_bnd, phonemes, arabic_dict = phonetise(in1)
    buckwalter = [arabic_to_buckwalter(x) for x in in1]
    transcript = [artr.transcript(x) for x in in1]

    result = []
    for i in range(len(transcript)):
        result.append({
            'arabic': in1[i],
            'transcripted': transcript[i],
            'buckwalter': buckwalter[i],
            'phonemes_with_bnd': phonemes_with_bnd[i],
            'phonemes': phonemes[i],
            'i': i
        })

    with open('out1.json', 'w') as f:
        json.dump(result, f, indent=4, ensure_ascii=False)
