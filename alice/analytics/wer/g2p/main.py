import argparse
import json
import os
import copy
import shutil
import subprocess
from alice.analytics.wer.lib.utils import get_phoneme_mapping, normalize_phonemes


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--transcriber')
    parser.add_argument('--input')
    parser.add_argument('--output')
    parser.add_argument('--extended', default=False, action='store_true')
    args = parser.parse_args()

    phoneme_mapping = get_phoneme_mapping()
    transcriber_binary = os.path.abspath(args.transcriber)
    config = os.path.abspath(os.path.join(os.getcwd(), 'conf/mt_transcribe.json'))
    config_copied = os.path.abspath(os.path.join(os.getcwd(), 'mt_transcribe_tmp.json'))
    shutil.copyfile(config, config_copied)
    src = os.path.abspath(args.input)
    dst = src + '_phonemes'
    # also need to have data folder with models in WD
    command = '{} -c {} -i - -o -'.format(transcriber_binary, config_copied)
    subprocess.run([transcriber_binary, '-c', config_copied, '-i', src, '-o', dst])

    out_data = []
    with open(dst, 'rt') as phonemes:
        for line in phonemes:
            res = {}
            words_list = []
            phonemes_list = []
            for word in json.loads(line):
                words_list.append(word["word"])
                phonemes_normalized = normalize_phonemes(phoneme_mapping, copy.deepcopy(word["phonemes"]))
                if args.extended:
                    phonemes_list.append(set(phonemes_normalized))
                else:
                    if phonemes_normalized:
                        most_frequent = max(set(phonemes_normalized), key=phonemes_normalized.count)
                        phonemes_list.append(most_frequent)
            res["text"] = ' '.join(words_list)
            if args.extended:
                res["phonemes"] = phonemes_list
            else:
                res["phonemes"] = list(' '.join(phonemes_list))
            out_data.append(res)

    with open(args.output, 'w') as f:
        json.dump(out_data, f, indent=4, ensure_ascii=False)

if __name__ == '__main__':
    main()
