import argparse
import os
import sys
import json
from alice.analytics.wer.lib.werp import WERP


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cwd')
    parser.add_argument('--input')
    parser.add_argument('--output')
    parser.add_argument('--ref', default='ref_text')
    parser.add_argument('--hyp', default='asr_text')
    args = parser.parse_args()

    out_data = []

    if args.input:
        with open(args.input, 'rt') as f:
            in_data = json.load(f)
    else:
        ref, hyp = sys.stdin.read().split('@')
        in_data = [{'ref_text': ref.strip(), 'asr_text': hyp.strip()}]

    if args.cwd:
        cwd = args.cwd
    else:
        cwd = os.getcwd()

    with WERP(cwd) as werp:
        for row in in_data:
            ref = row[args.ref]
            hyp = row[args.hyp]
            row['scores'] = werp(ref, hyp)
            out_data.append(row)

    if args.output:
        with open(args.output, 'w') as f:
            json.dump(out_data, f, indent=4, ensure_ascii=False)
    else:
        print(out_data)

# ./werp --cwd ~/arc/arcadia/dict/mt/g2p/transcriber/tool --input ./werp_test.json --output ./werp_test_out.json
# or
# echo niletto@нилетто | ./werp --cwd ~/arc/arcadia/dict/mt/g2p/transcriber/tool
