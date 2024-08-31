import sys

try:
    import gensim
except ImportError:
    print "Please, install gensim from dev-requirements.txt"
    sys.exit()

import codecs
import os
from marisa_trie import BytesTrie
import click
import numpy as np

from vins_core.nlu.features.extractor.embeddings_ids import EmbeddingsMapIds


class MySentences(object):
    LOG_EVERY = 1000000

    def __init__(self, directory, start_prefix=None, verbose=True):
        self._dir = os.path.expanduser(directory)
        self._start_prefix = start_prefix
        self._verbose = verbose

    def __iter__(self):
        fnames = os.listdir(self._dir)
        if self._start_prefix:
            fnames = list(filter(lambda x: x.startswith(self._start_prefix), fnames))
        fnames = [os.path.join(self._dir, fname) for fname in fnames]

        cnt = 0
        for fname in fnames:
            with codecs.open(fname, encoding='utf8') as fin:
                for line in fin:
                    yield line.strip().split()
                    cnt += 1
                    if cnt % self.LOG_EVERY == 0 and self._verbose:
                        print "On line #{}".format(cnt)


def iter_word_and_vector_w2v(word2vec, tobytes=True):
    for id_, word in enumerate(word2vec.wv.index2word):
        emb = word2vec[word]
        if tobytes:
            emb = emb.astype(np.float32).tobytes()

        yield unicode(word), emb


def iter_word_and_vector_trie(trie):
    for word, embedding in trie.iteritems():
        yield word, np.fromstring(embedding, dtype=np.float32)


def convert_to_trie(word2vec, trie_name):
    trie = BytesTrie(iter_word_and_vector_w2v(word2vec))
    trie.save(trie_name)


@click.group()
def main():
    pass


@main.command('train', short_help='Train word2vec. Input files should be in the format of'
                                  'lines with sentences.\n\n'
                                  'Usage: python train_w2v.py '
                                  '--size=300 --window=2 --workers=16 '
                                  '--data_dir=/home/username/dirwithdata/ '
                                  '--files_prefix=w2v_file_')
@click.option('--data_dir', type=click.Path(), help='Path to data dir.')
@click.option('--files_prefix', default='', help='Specifies files prefix. '
                                                 'Default=process all files in --data_dir.')
@click.option('--size', default=300, help='Size of embeddings. Default=300.')
@click.option('--window', type=int, default=3, help='Context window size. Default=3.')
@click.option('--sg', type=int, default=0, help='Learning method. 0 means cbow,'
                                                '1 means skip-gram. Default=cbow.')
@click.option('--min_count', type=int, default=3, help='Vocabulary word minimum frequency. Default=3.')
@click.option('--workers', type=int, default=4, help='Num of workers. As greater as faster. Default=4.')
@click.option('--iter', type=int, default=5, help='Number of epochs. Default=5.')
@click.option('--verbose', is_flag=True, default=True, help='Verbosity flag. Default=True.')
def train(data_dir, files_prefix, size, window, sg, min_count, workers, iter, verbose):
    print "Train word2vec with args:"
    print "--size=%d" % size
    print "--window=%d" % window
    print "--sg=%d" % sg
    print "--min_count=%d" % min_count
    print "--workers=%d" % workers
    print "--iter=%d" % iter
    print "--data_dir=%s" % data_dir
    print "--files_prefix=%s" % files_prefix
    print "--verbose=%d" % verbose

    sentences = MySentences(data_dir, files_prefix, verbose)

    print "Start training..."
    word2vec = gensim.models.word2vec.Word2Vec(sentences, size=size, window=window,
                                               sg=sg, min_count=min_count,
                                               workers=workers, iter=iter)
    modelname_base = '_'.join(['size', str(size),
                               'window', str(window),
                               'sg', str(sg),
                               'min_count', str(min_count),
                               'iter', str(iter)])

    modelname = 'w2v_' + modelname_base + '.model'
    triename = 'trie_' + modelname_base
    idsname = 'ids_{}'.format(modelname_base)

    print "Save w2v model to '%s'" % modelname
    word2vec.save(modelname)

    print "Save marisa-trie to '%s'" % triename
    convert_to_trie(word2vec, triename)

    print "Save ids marisa-trie and embeddings matrix to {}\n".format(idsname)
    EmbeddingsMapIds.dump_binary(idsname, iter_word_and_vector_w2v(word2vec, tobytes=False))

    print "You can now pass 'embeddings_file={0}' argument to create_token_classifier" \
          "and get your embeddings.".format(triename)
    print "Or you can upload them to sandbox with 'ya upload {0}'" \
          " and use 'embeddings_resource' argument.".format(triename)
    print "Good luck!"


@main.command('convert2ids', short_help='Convert from format for "embeddings" feature extractor to format '
                                        'for "embeddings_ids feature extractor"')
@click.option('--trie_file', type=click.Path(), help='File with BytesTrie marisa-trie :: token -> embedding')
def convert2ids(trie_file):
    idsname = 'ids_{}'.format(os.path.basename(trie_file))
    trie = BytesTrie().load(trie_file)

    EmbeddingsMapIds.dump_binary(idsname, iter_word_and_vector_trie(trie))

    print "Embeddings converted"


if __name__ == '__main__':
    sys.exit(main())
