import argparse
import json
import pickle
import pymorphy2
import traceback
from tqdm import tqdm

from voicetech.asr.tools.metrics.lib.metric import WerpMetric
from voicetech.asr.tools.metrics.lib.record import Sample, Record, Response
from alice.analytics.wer.lib import utils


def _parse_args():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument(
        '--input',
        help='Input file with textes, one line - one text')
    parser.add_argument(
        '--output',
        help='Output pkl file with cache as dict')    

    return parser.parse_args()


def main():
    args = _parse_args()
    metric = WerpMetric()
    werp = metric._computer._computer
    cached = set()
    g2p_static_cache = {}
    with open(args.input, 'r', encoding='UTF8') as fin:
        for line in tqdm(fin):
            text = line.strip()
            reprs = {'original': text}
            reprs['normalized'] = utils.normalize_words(werp.morph, text)
            reprs['meaningful'] = werp.get_meaningful_text(text)
            phoneme_repr_types = ['original', 'normalized', 'meaningful']
            for repr_type in phoneme_repr_types:
                line_to_send = reprs[repr_type]

                if not line_to_send:
                    continue

                for word in line_to_send.split():
                    if word in cached:
                        continue
                    cached.add(word)
                    ans = werp.g2p_applier.transcribe(word)
                    g2p_static_cache[word] = ans[0]

    pickle.dump(g2p_static_cache, open(args.output, 'wb'))
