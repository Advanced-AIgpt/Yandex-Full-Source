# coding: utf-8

import argparse
import time

from vins_core.nlu.features.extractor.embeddings import EmbeddingsMap


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--input-file', metavar='FILE', dest='input_file', required=True,
                        help='A .txt file with embeddings')
    parser.add_argument('--output-file', metavar='FILE', dest='output_file', required=True,
                        help='The resulting .pkl file with embeddings in binary format')
    args = parser.parse_args()

    load_start = time.time()
    embeddings_map = EmbeddingsMap.load_from_text_file(args.input_file)
    print 'Text file loading took %.1fs' % (time.time() - load_start)

    embeddings_map.save_to_bin_file(args.output_file)

    load_start = time.time()
    EmbeddingsMap.load_from_bin_file(args.output_file)
    print 'Binary file loading took %.1fs' % (time.time() - load_start)


if __name__ == '__main__':
    main()
