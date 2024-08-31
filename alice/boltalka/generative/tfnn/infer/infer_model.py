import argparse
import json
import re
import time

from tokenizer import Tokenizer

from alice.boltalka.generative.tfnn.infer_lib.infer import load_model, infer_model
from alice.boltalka.generative.tfnn.preprocess import preprocess_contexts, preprocess_contexts_and_tokenize


def main():
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--full-bpe-voc')
    group.add_argument('--pretokenized-input', action='store_true')

    parser.add_argument('--mode', default='sample')
    parser.add_argument('--model', required=True)
    parser.add_argument('--model-path', required=True)
    parser.add_argument('--ivoc', required=True)
    parser.add_argument('--ovoc', required=True)
    parser.add_argument('--hp', type=lambda x: json.loads(x), required=True)
    parser.add_argument('--model-prefix-name', default='mod')
    parser.add_argument('--device', default='')

    parser.add_argument('--sampling-temperature', default=0.6, type=float)
    parser.add_argument('--sampling-topk', default=50, type=int)
    parser.add_argument('--sampling-hypothesis-per-input', default=1, type=int)

    parser.add_argument('--infer-model-kwargs', type=json.loads, default={})
    parser.add_argument('--skip-preprocessing', action='store_true')

    args = parser.parse_args()

    model, session, graph = load_model(
        args.model, args.model_path, args.ivoc, args.ovoc, args.hp,
        model_prefix_name=args.model_prefix_name, device=args.device
    )
    if not args.pretokenized_input:
        tokenizer = Tokenizer(args.full_bpe_voc.encode('utf-8'))

    with session.as_default(), graph.as_default():
        while True:
            console_input = input()

            input_strings = console_input.split('\t')
            inputs_tokenized = input_strings
            if not args.pretokenized_input:
                inputs_tokenized = [
                    preprocess_contexts_and_tokenize(
                        [string], separator_token=None, tokenizer=tokenizer, skip_preprocessing=args.skip_preprocessing
                    )
                    for string in input_strings
                ]

            print('Inputs:')
            print(inputs_tokenized)

            start_time = time.time()
            results = infer_model(
                model,
                [inputs_tokenized],
                temperature=args.sampling_temperature,
                sampling_top_k=args.sampling_topk,
                hypothesis_per_input=args.sampling_hypothesis_per_input,
                mode=args.mode,
                batch_size=1,
                unbpe=True,
                **args.infer_model_kwargs
            )

            print('Time: {:.2f} seconds'.format(time.time() - start_time))
            print(len(results))
            for result in results:
                print('Output:')
                print(result)


if __name__ == '__main__':
    main()
