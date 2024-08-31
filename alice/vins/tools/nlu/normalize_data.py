from __future__ import unicode_literals
import argparse
import codecs
import os
import multiprocessing as mp
import functools

from vins_core.nlu.samples_extractor import SamplesExtractor
from vins_core.nlu.sample_processors.registry import create_sample_processor
from vins_core.ner.fst_normalizer import DEFAULT_RU_NORMALIZER_NAME


def normalize(input_output, normalizer):
    input_file, output_file = input_output
    with codecs.open(input_file, encoding='utf8') as fin, codecs.open(output_file, 'w', encoding='utf8') as fout:
        for line in fin:
            fout.write(' '.join(normalizer([line], filter_errors=True)[0].tokens) + '\n')


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Normalize many small text files using VINS normalizer/tokenizer.\n'
                                                 'For each small chunk with name "some_name.txt" creates normalized '
                                                 'chunk with name "normed_some_name.txt"\n\n'
                                                 'If you have just ONE HUGE file, split it using unix command\n'
                                                 '"split -l 100000 YOUR_HUGE_FILE.txt SMALL_FILES_PREFIX_" '
                                                 '(-l means number lines in each chunk) or\n'
                                                 '"split -n 50 YOUR_HUGE_FILE.txt SMALL_FILES_PREFIX_" '
                                                 '(-n means number of chunks).\n\n'
                                                 'Usage: python normalize_data.py '
                                                 '--data_dir=/home/username/dirwithdata/ '
                                                 '--files_prefix=my_files_prefix_ '
                                                 '--out_dir=/home/username/outdir/ '
                                                 '--workers=48',
                                     formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument('--data_dir', type=str, help='Path to data dir.')
    parser.add_argument('--files_prefix', type=str, default='', help='Specifies files prefix. '
                                                                     'Default=Process all files in --data_dir.')
    parser.add_argument('--workers', type=int, default=4, help='num of workers. default=4')
    parser.add_argument('--normalizer', type=str, default=DEFAULT_RU_NORMALIZER_NAME,
                        help='Name of normalizer in NormalizeSampleProcessor.'
                             'Default=\'%s\'.' % DEFAULT_RU_NORMALIZER_NAME)

    args = parser.parse_args()

    data_dir = os.path.expanduser(args.data_dir)
    print "Normalize files using args:"
    print "--data_dir=%s" % data_dir
    print "--files_prefix=%s" % args.files_prefix
    print "--workers=%d" % args.workers
    print "--normalizer=%s" % args.normalizer

    normalizer = SamplesExtractor([create_sample_processor('normalizer', normalizer=args.normalizer)])
    worker_func = functools.partial(normalize, normalizer=normalizer)

    chunks_fnames = filter(lambda x: x.startswith(args.files_prefix), os.listdir(data_dir))

    input_fnames = [os.path.join(data_dir, chunk) for chunk in chunks_fnames]
    output_fnames = [os.path.join(data_dir, 'normed_'+chunk) for chunk in chunks_fnames]

    print 'Created Pool with %d workers.' % args.workers
    pool = mp.Pool(args.workers)

    print 'Start normalizing! You can track progress using <watch -n 1 "wc -l normed_{}*">.'.format(args.files_prefix)
    pool.map(worker_func, zip(input_fnames, output_fnames))

    pool.close()
    pool.join()
